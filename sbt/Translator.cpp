#include "Translator.h"

#include "Builder.h"
#include "Disassembler.h"
#include "Instruction.h"
#include "Module.h"
#include "XRegisters.h"
#include "SBTError.h"
#include "Stack.h"
#include "Syscall.h"
#include "Utils.h"

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
    // stack
    _ctx->stack = new Stack(_ctx);
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
    _iCaller.create(llvm::FunctionType::get(t.i32, args, !VAR_ARG));

    _isExternal.create(llvm::FunctionType::get(t.i32, { t.i32 }, !VAR_ARG));

    // SBT abort, using syscalls only
    // - to abort on fatal errors such as: icaller could not resolve guest
    //   address to host address
    // - makes debugging easier
    // - doesn't depend on C environment
    _sbtabort.reset(new Function(_ctx, "sbtabort"));
    _sbtabort->create();

    return llvm::Error::success();
}


llvm::Error Translator::startTarget()
{
    SBTError serr;

    // get target
    llvm::Triple triple("riscv32-unknown-elf");
    std::string tripleName = triple.getTriple();

    std::string strError;
    _target = llvm::TargetRegistry::lookupTarget(tripleName, strError);
    if (!_target) {
        serr << "target not found: " << tripleName;
        return error(serr);
    }

    _mri.reset(_target->createMCRegInfo(tripleName));
    if (!_mri) {
        serr << "no register info for target " << tripleName;
        return error(serr);
    }

    // set up disassembler.
    _asmInfo.reset(_target->createMCAsmInfo(*_mri, tripleName));
    if (!_asmInfo) {
        serr << "no assembly info for target " << tripleName;
        return error(serr);
    }

    llvm::SubtargetFeatures features("+m");
    _sti.reset(
            _target->createMCSubtargetInfo(tripleName, "", features.getString()));
    if (!_sti) {
        serr << "no subtarget info for target " << tripleName;
        return error(serr);
    }

    _mofi.reset(new llvm::MCObjectFileInfo);
    _mc.reset(new llvm::MCContext(_asmInfo.get(), _mri.get(), _mofi.get()));
    _disAsm.reset(_target->createMCDisassembler(*_sti, *_mc));
    if (!_disAsm) {
        serr << "no disassembler for target " << tripleName;
        return error(serr);
    }

    _mii.reset(_target->createMCInstrInfo());
    if (!_mii) {
        serr << "no instruction info for target " << tripleName;
        return error(serr);
    }

    _instPrinter.reset(
        _target->createMCInstPrinter(triple, 0, *_asmInfo, *_mii, *_mri));
    if (!_instPrinter) {
        serr << "no instruction printer for target " << tripleName;
        return error(serr);
    }

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


llvm::Expected<uint64_t> Translator::import(const std::string& func)
{
    // check if the function was already processed
    if (auto f = _funMap[func])
        return (*f)->addr();

    // load libc module
    if (!_lcModule) {
        const auto& libcBC = _ctx->c.libCBC();
        if (libcBC.empty()) {
            SBTError serr;
            serr << "libc.bc file not found";
            return error(serr);
        }

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
    llvm::Function* lf = _lcModule->getFunction(func);
    if (!lf) {
        FunctionNotFound serr;
        serr << __FUNCTION__ << "(): function not found: " << func;
        return error(serr);
    }
    llvm::FunctionType* ft = lf->getFunctionType();

    xassert(ft->getNumParams() < 9 &&
        "external functions with more than 8 params are not supported!");

    // set a fake guest address for the external function
    uint64_t addr = _extFuncAddr;
    DBGF("{0:X+8} -> {1}", addr, func);

    // declare imported function in our module
    Function* f = new Function(_ctx, func, nullptr, addr);
    FunctionPtr fp(f);
    f->create(ft);

    // add to maps
    _funcByAddr(_extFuncAddr, std::move(f));
    _funMap(f->name(), std::move(fp));
    _extFuncAddr += Constants::INSTRUCTION_SIZE;

    return addr;
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
    bld->call(_sbtabort->func());
    bld->ret(c.ZERO);

    // cases
    // case fun: arg = realFunAddress;
    for (const auto& p : _funcByAddr) {
        Function* f = p.val;
        uint64_t addr = f->addr();
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

        // set args
        auto argit = ic->arg_begin();
        argit++;    // skip target
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

                v = bld->bitOrPointerCast(v, ty);
            }

            args.push_back(v);
        }

        // call
        llvm::PointerType* ty = llft->getPointerTo();
        llvm::Value* fptr = bld->bitOrPointerCast(sym, ty);
        llvm::Value* ret;
        /*
        DBGF("ft:");
        llft->dump();
        DBGF("args:");
        for (auto arg : args)
            arg->dump();
        */
        if (llft->getReturnType()->isVoidTy()) {
            bld->call(fptr, args);
            ret = _ctx->c.ZERO;
        } else {
            ret = bld->call(fptr, args);
            if (ret->getType() != t.i32)
                ret = bld->bitOrPointerCast(ret, t.i32);
        }
        bld->ret(ret);

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
