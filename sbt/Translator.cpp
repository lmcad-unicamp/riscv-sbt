#include "Translator.h"

#include "AddressToSource.h"
#include "Builder.h"
#include "Disassembler.h"
#include "FRegister.h"
#include "Instruction.h"
#include "Module.h"
#include "SBTError.h"
#include "Stack.h"
#include "Syscall.h"
#include "Utils.h"
#include "XRegister.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>

#include <map>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"


namespace sbt {

static Context* setOpts(Context* ctx, Options* opts)
{
    // opts
    ctx->opts = opts;
    return ctx;
}


Translator::Translator(Context* ctx)
    :
    _ctx(sbt::setOpts(ctx, &_opts)),
    _iCaller(_ctx, "rv32_icaller"),
    _isExternal(_ctx, "rv32_isExternal")
{
    _ctx->translator = this;
}


Translator::~Translator()
{
}


llvm::Error Translator::start()
{
    if (auto err = startTarget())
        return err;

    // setup context

    // global register file
    _ctx->x = new XRegisters(_ctx, XRegisters::NONE);
    _ctx->f = new FRegisters(_ctx, FRegisters::NONE);
    // stack
    _ctx->stack = new Stack(_ctx, _opts.stackSize());
    // disassembler
    _ctx->disasm = new Disassembler(&*_disAsm, &*_instPrinter, &*_sti);
    // function lookup
    _ctx->_func = &_funMap;
    _ctx->_funcByAddr = &_funcByAddr;

    // declare icaller
    const Types& t = _ctx->t;
    std::vector<llvm::Type*> args;
    const size_t n = 9;
    args.reserve(n);
    for (size_t i = 0; i < n; i++)
        args.push_back(t.i32);
    _iCaller.create(llvm::FunctionType::get(t.voidT, args, !VAR_ARG));

    _isExternal.create(llvm::FunctionType::get(t.i32, { t.i32 }, !VAR_ARG));

    // SBT abort, using syscalls only
    // - to abort on fatal errors such as: icaller could not resolve guest
    //   address to host address
    // - makes debugging easier
    // - doesn't depend on C environment
    _sbtabort.reset(new Function(_ctx, "sbtabort"));
    _sbtabort->create();

    // address to source
    auto expA2S = sbt::create<AddressToSource*>(_opts.a2s());
    if (!expA2S)
        return expA2S.takeError();
    _a2s.reset(expA2S.get());
    _ctx->a2s = &*_a2s;

    return llvm::Error::success();
}


llvm::Error Translator::startTarget()
{
    // get target
    llvm::Triple triple("riscv32-unknown-elf");
    std::string tripleName = triple.getTriple();

    std::string strError;
    _target = llvm::TargetRegistry::lookupTarget(tripleName, strError);
    if (!_target)
        return ERROR(llvm::formatv("target not found: {0}", tripleName));

    _mri.reset(_target->createMCRegInfo(tripleName));
    if (!_mri)
        return ERROR(llvm::formatv("no register info for target {0}", tripleName));

    // set up disassembler.
    _asmInfo.reset(_target->createMCAsmInfo(*_mri, tripleName));
    if (!_asmInfo)
        return ERROR(llvm::formatv("no assembly info for target {0}", tripleName));

    llvm::SubtargetFeatures features("-a,-c,+m,+f,+d");
    _sti.reset(
        _target->createMCSubtargetInfo(tripleName, "", features.getString()));
    if (!_sti)
        return ERROR(llvm::formatv("no subtarget info for target {0}", tripleName));

    _mofi.reset(new llvm::MCObjectFileInfo);
    _mc.reset(new llvm::MCContext(_asmInfo.get(), _mri.get(), _mofi.get()));
    _disAsm.reset(_target->createMCDisassembler(*_sti, *_mc));
    if (!_disAsm)
        return ERROR(llvm::formatv("no disassembler for target {0}", tripleName));

    _mii.reset(_target->createMCInstrInfo());
    if (!_mii)
        return ERROR(llvm::formatv("no instruction info for target {0}",tripleName));

    _instPrinter.reset(
        _target->createMCInstPrinter(triple, 0, *_asmInfo, *_mii, *_mri));
    if (!_instPrinter)
        return ERROR(llvm::formatv("no instruction printer for target {0}", tripleName));

    return llvm::Error::success();
}


llvm::Error Translator::finish()
{
    genIsExternal();
    genICaller();
    return llvm::Error::success();
}


Syscall& Translator::syscall()
{
    // create handler on first use (if any)
    if (!_sc) {
        _sc.reset(new Syscall(_ctx));
        _sc->genHandler();
    }

    return *_sc;
}


void Translator::initCounters()
{
    if (_initCounters) {
        llvm::Function* f = llvm::Function::Create(_ctx->t.voidFunc,
            llvm::Function::ExternalLinkage, "counters_init", _ctx->module);
        _ctx->bld->call(f);

        llvm::FunctionType *ft = llvm::FunctionType::get(_ctx->t.i32, !VAR_ARG);

        _getCycles.reset(new Function(_ctx, "get_cycles"));
        _getTime.reset(new Function(_ctx, "get_time"));
        _getInstRet.reset(new Function(_ctx, "get_instret"));

        _getCycles->create(ft);
        _getTime->create(ft);
        _getInstRet->create(ft);

        _initCounters = false;
    }
}


llvm::Error Translator::translate()
{
    _opts.dump();
    DBGS << "input files:";
    for (const auto& f : _inputFiles)
        DBGS << ' ' << f;
    DBGS << nl << "output file: " << _outputFile << nl;

    if (auto err = start())
        return err;

    for (const auto& f : _inputFiles) {
        Module mod(_ctx);
        if (auto err = mod.translate(f))
            return err;
    }

    if (auto err = finish())
        return err;

    return llvm::Error::success();
}

static std::map<std::string, std::string> g_funcSubst = {
    {"__addtf3", "sbt__addtf3"},
    {"__subtf3", "sbt__subtf3"},
    {"__multf3", "sbt__multf3"},
    {"__divtf3", "sbt__divtf3"},
    {"__extenddftf2", "sbt__extenddftf2"},
    {"__trunctfdf2",  "sbt__trunctfdf2"},
    {"__lttf2",  "sbt__lttf2"}
};

llvm::Expected<std::pair<uint64_t, std::string>>
Translator::import(const std::string& func)
{
    using RetT = llvm::Expected<std::pair<uint64_t, std::string>>;
    auto make_ret = [](uint64_t addr, const std::string& xfunc) {
        return RetT(std::pair<uint64_t, std::string>(addr, xfunc));
    };

    // replace by another function, if needed
    std::string xfunc;
    auto it = g_funcSubst.find(func);
    if (it != g_funcSubst.end())
        xfunc = it->second;
    else
        xfunc = func;

    // check if the function was already processed
    if (auto f = _funMap[xfunc])
        return make_ret((*f)->addr(), xfunc);

    // load libc module
    if (!_lcModule) {
        const auto& libcBC = _ctx->c.libCBC();
        if (libcBC.empty())
            return ERROR("libc.bc file not found");

        auto res = llvm::MemoryBuffer::getFile(libcBC);
        if (!res)
            return llvm::errorCodeToError(res.getError());
        llvm::MemoryBufferRef buf = **res;

        llvm::LLVMContext* ctx = _ctx->ctx;
        auto expMod = llvm::parseBitcodeFile(buf, *ctx);
        if (!expMod)
            return expMod.takeError();
        _lcModule = std::move(*expMod);
    }

    // lookup function
    llvm::Function* lf = _lcModule->getFunction(xfunc);
    if (!lf) {
        // check if its data
        if (_lcModule->getNamedGlobal(xfunc))
            return make_ret(SYM_TYPE_DATA, xfunc);
        return ERROR2(FunctionNotFound,
            llvm::formatv("function not found: {0}", xfunc));
    }
    llvm::FunctionType* ft = lf->getFunctionType();

    xassert(ft->getNumParams() < 9 &&
        "external functions with more than 8 params are not supported!");

    // set a fake guest address for the external function
    uint64_t addr = _extFuncAddr;
    DBGF("{0:X+8} -> {1}", addr, xfunc);

    // declare imported function in our module
    Function* f = new Function(_ctx, xfunc, nullptr, addr);
    FunctionPtr fp(f);
    f->create(ft);

    // add to maps
    _funcByAddr(_extFuncAddr, std::move(f));
    _funMap(f->name(), std::move(fp));
    _extFuncAddr += Constants::INSTRUCTION_SIZE;

    return make_ret(addr, xfunc);
}


llvm::Value*
Translator::i32x2ToFP64(Builder* bld, llvm::Value* lo, llvm::Value* hi)
{
    llvm::Value* vlo = bld->zext64(lo);
    llvm::Value* vhi = bld->zext64(hi);
    vhi = bld->sll(vhi, _ctx->c.i64(32));
    // merge hi and low
    llvm::Value* v = bld->_or(vhi, vlo);
    // then finally cast to double
    v = bld->bitOrPointerCast(v, _ctx->t.fp64);
    return v;
}


std::pair<llvm::Value*, llvm::Value*>
Translator::fp64ToI32x2(Builder* bld, llvm::Value* f)
{
    llvm::Value* v = bld->bitOrPointerCast(f, _ctx->t.i64);
    llvm::Value* vlo = bld->truncOrBitCastI32(v);
    llvm::Value* vhi = bld->srl(v, _ctx->c.i64(32));
    vhi = bld->truncOrBitCastI32(vhi);
    return std::make_pair(vlo, vhi);
}


llvm::Value* Translator::refToFP128(Builder* bld, llvm::Value* ref)
{
    llvm::Value* ptr = bld->bitOrPointerCast(ref, _ctx->t.fp128ptr);
    return bld->load(ptr);
}


void Translator::fp128ToRef(Builder* bld, llvm::Value* f, llvm::Value* ref)
{
    llvm::Value* ptr = bld->bitOrPointerCast(ref, _ctx->t.fp128ptr);
    bld->store(f, ptr);
}


void Translator::genICaller()
{
    DBGF("entry");

    // prepare
    const Constants& c = _ctx->c;
    const Types& t = _ctx->t;
    Builder bldi(_ctx, NO_FIRST);
    Builder* bld = &bldi;
    llvm::Function* ic = _iCaller.func();
    const size_t maxArgs = ic->arg_size() - 1;
    xassert(ic);

    xassert(!ic->arg_empty());
    llvm::Argument& target = *ic->arg_begin();

    // basic blocks
    BasicBlock bbBeg(_ctx, "begin", ic);
    BasicBlock bbDfl(_ctx, "default", ic);

    // begin: switch
    bld->setInsertBlock(&bbBeg);
    llvm::SwitchInst* sw = bld->sw(&target, bbDfl, _funMap.size());

    // default: abort

    bld->setInsertBlock(&bbDfl);

    // print error msg
    if (_opts.useLibC()) {
        // declare printf
        llvm::FunctionType* ft_printf =
            llvm::FunctionType::get(_ctx->t.i32, { _ctx->t.i8ptr }, VAR_ARG);
        FunctionPtr f_printf(new Function(_ctx, "printf"));
        f_printf->create(ft_printf);

        // args
        std::vector<llvm::Value*> args;
        // fmt
        std::string fmt = "FATAL ERROR: at icaller: function not found: %d\n";
        llvm::ArrayRef<uint8_t> fmtA(
        reinterpret_cast<const uint8_t*>(fmt.data()), fmt.size());
        llvm::Constant* fmtC = llvm::ConstantDataArray::get(*_ctx->ctx, fmtA);
        auto fmtGV = new llvm::GlobalVariable(
            *_ctx->module, fmtC->getType(), CONSTANT,
            llvm::GlobalValue::InternalLinkage, fmtC, "icaller_error_fmt");
        // get pointer to fmt
        llvm::Value* v = bld->gep(fmtGV, { c.ZERO, c.ZERO });
        args.push_back(v);
        // target
        args.push_back(&target);

        // call it
        bld->call(f_printf->func(), args);
    }

    // abort
    bld->call(_sbtabort->func());
    bld->retVoid();

    // cases
    // case fun: arg = realFunAddress;
    for (const auto& p : _funcByAddr) {
        Function* f = p.val;
        uint64_t addr = f->addr();
        bool isExternal = addr >= FIRST_EXT_FUNC_ADDR;
        DBGF("function={0}, addr={1:X+8}", f->name(), addr);
        xassert(addr != Constants::INVALID_ADDR);

        // get function by symbol
        // XXX this may fail for internal functions
        std::string caseStr = "case_" + f->name();
        llvm::Value* sym =
            _ctx->module->getValueSymbolTable().lookup(f->name());
        xassert(sym);

        // prepare
        llvm::Function* llf = f->func();
        llvm::FunctionType* llft = llf->getFunctionType();
        size_t fixedArgs = llft->getNumParams();
        size_t totalArgs;
        // varArgs: passing 4 extra args for now
        if (llft->isVarArg())
            totalArgs = MIN(fixedArgs + 4, maxArgs);
        else
            totalArgs = fixedArgs;
        std::vector<llvm::Value*> args;
        args.reserve(totalArgs);

        // BB
        BasicBlock dest(_ctx, caseStr, ic, bbDfl.bb());
        bld->setInsertBlock(&dest);

        // XXX skip main for now
        if (f->name() == "main") {
            bld->br(bbDfl);
            bld->setInsertBlock(&bbBeg);
            sw->addCase(c.i32(addr), dest.bb());
            continue;
        }

        // get pointer to function
        llvm::PointerType* fty = llft->getPointerTo();
        llvm::Value* fptr = bld->bitOrPointerCast(sym, fty);
        // get return type
        llvm::Type* retty = llft->getReturnType();
        llvm::Value* retref = nullptr;
        // get args
        auto argit = ic->arg_begin();
        ++argit;    // skip target

        // types > 2xi32 are returned by ref and the address is passed by the
        // caller in the first argument
        if (retty->isFP128Ty())
            retref = &*argit++;

        // set args
        for (size_t i = 0; i < totalArgs; i++, ++argit) {
            llvm::Value* v = &*argit;
            llvm::Type* ty = i < fixedArgs? llft->getParamType(i) : t.i32;

            // need to cast?
            if (ty != t.i32) {
                DBGF("cast from:");
                DBG(v->getType()->dump());
                DBGF("cast to:");
                DBG(ty->dump());
                DBGS.flush();

                if (ty->isDoubleTy())
                    v = i32x2ToFP64(bld, v, &*++argit);
                // long double: by ref
                else if (ty->isFP128Ty())
                    v = refToFP128(bld, v);
                else
                    v = bld->bitOrPointerCast(v, ty);
            }

            args.push_back(v);
        }

        // call
        if (retty->isVoidTy()) {
            bld->call(fptr, args);
        // write return value into x10/x11 for libc functions
        } else {
            // ret: ret will be set if value is returned by ref
            xassert(isExternal);
            llvm::Value* ret = bld->call(fptr, args);
            if (retty->isDoubleTy()) {
                auto p = fp64ToI32x2(bld, ret);
                auto& reglo = _ctx->x->getReg(XRegister::A0);
                auto& reghi = _ctx->x->getReg(XRegister::A1);
                bld->store(p.first, reglo.getForWrite());
                bld->store(p.second, reghi.getForWrite());
            } else if (retty->isFP128Ty()) {
                xassert(retref);
                fp128ToRef(bld, ret, retref);
            } else {
                if (retty != t.i32)
                    ret = bld->bitOrPointerCast(ret, t.i32);
                auto& reg = _ctx->x->getReg(XRegister::A0);
                bld->store(ret, reg.getForWrite());
            }
        }
        bld->retVoid();

        bld->setInsertBlock(&bbBeg);
        sw->addCase(c.i32(addr), dest.bb());
    }
}


void Translator::genIsExternal()
{
    DBGF("entry");

    // prepare
    const Constants& c = _ctx->c;
    Builder bldi(_ctx, NO_FIRST);
    Builder* bld = &bldi;
    llvm::Function* ie = _isExternal.func();
    xassert(ie);

    xassert(!ie->arg_empty());
    llvm::Argument& target = *ie->arg_begin();

    // basic blocks
    BasicBlock bbEntry(_ctx, "ie_entry", ie);
    BasicBlock bbTrue(_ctx, "ie_true", ie);
    BasicBlock bbFalse(_ctx, "ie_false", ie);

    bld->setInsertBlock(&bbEntry);
    llvm::Value* cond = bld->uge(&target, c.u32(FIRST_EXT_FUNC_ADDR));
    bld->condBr(cond, &bbTrue, &bbFalse);

    bld->setInsertBlock(&bbTrue);
    bld->ret(c.i32(1));

    bld->setInsertBlock(&bbFalse);
    bld->ret(c.ZERO);
}

} // sbt
