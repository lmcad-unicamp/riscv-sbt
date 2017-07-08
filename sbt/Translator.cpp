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

Translator::Translator(Context* ctx)
    :
    _ctx(ctx),
    _iCaller(_ctx, "rv32_icaller")
{
    _ctx->translator = this;
}


Translator::~Translator()
{
}


llvm::Error Translator::start()
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

    llvm::SubtargetFeatures features;
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

    // setup context

    _ctx->x = new XRegisters(_ctx, DECL);
    _ctx->stack = new Stack(_ctx);
    _ctx->disasm = new Disassembler(&*_disAsm, &*_instPrinter, &*_sti);
    _ctx->_func = &_funMap;
    _ctx->_funcByAddr = &_funcByAddr;

    const auto& t = _ctx->t;
    _iCaller.create(llvm::FunctionType::get(t.voidT, { t.i32 }, !VAR_ARG));

    // host functions

    llvm::FunctionType *ft = llvm::FunctionType::get(_ctx->t.i32, !VAR_ARG);

    _sbtabort.reset(new Function(_ctx, "sbtabort"));
    _getCycles.reset(new Function(_ctx, "get_cycles"));
    _getTime.reset(new Function(_ctx, "get_time"));
    _getInstRet.reset(new Function(_ctx, "get_instret"));

    _sbtabort->create();
    _getCycles->create(ft);
    _getTime->create(ft);
    _getInstRet->create(ft);

    // syscall handler
    _sc.reset(new Syscall(_ctx));
    _ctx->syscall = &*_sc;

    return llvm::Error::success();
}


llvm::Error Translator::finish()
{
    return genICaller();
}


llvm::Error Translator::genSCHandler()
{
    _ctx->x = new XRegisters(_ctx, !DECL);

    if (auto err = Syscall(_ctx).genHandler())
        return err;

    return llvm::Error::success();
}


llvm::Error Translator::translate()
{
    DBGS << "input files:";
    for (const auto& f : _inputFiles)
        DBGS << ' ' << f;
    DBGS << nl << "output file: " << _outputFile << nl;

    if (auto err = start())
        return err;

    _sc->declHandler();

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
    DBGF("enter");
    std::string rv32Func = "rv32_" + func;

    auto& ctx = *_ctx->ctx;

    // check if the function was already processed
    if (auto f = _funMap[rv32Func])
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

        auto expMod = llvm::parseBitcodeFile(buf, ctx);
        if (!expMod)
            return expMod.takeError();
        _lcModule = std::move(*expMod);
    }

    // lookup function
    llvm::Function* lf = _lcModule->getFunction(func);
    if (!lf) {
        SBTError serr;
        serr << "Function not found: " << func;
        return error(serr);
    }
    llvm::FunctionType* ft = lf->getFunctionType();

    Builder* bld = _ctx->bld;
    xassert(bld && "bld is null");
    auto module = _ctx->module;
    auto& t = _ctx->t;

    // declare imported function in our module
    llvm::Function* impf =
        llvm::Function::Create(ft,
                llvm::GlobalValue::ExternalLinkage, func, module);

    // create our caller to the external function
    DBGF("create caller");
    if (!_extFuncAddr)
        _extFuncAddr = 0xFFFF0000;
    uint64_t addr = _extFuncAddr;
    Function* f = new Function(_ctx, rv32Func, nullptr, addr);
    FunctionPtr fp(f);
    f->create(t.voidFunc, llvm::Function::PrivateLinkage);
    // add to maps
    _funcByAddr(_extFuncAddr, std::move(f));
    _funMap(f->name(), std::move(fp));
    _extFuncAddr += Instruction::SIZE;

    BasicBlock bb(_ctx, "entry", f->func());
    BasicBlock* prevBB = bld->getInsertBlock();
    bld->setInsertPoint(bb);

    OnScopeExit restoreInsertPoint(
        [&bld, &prevBB]() {
            bld->setInsertPoint(prevBB);
        });

    unsigned numParams = ft->getNumParams();
    xassert(numParams < 9 &&
            "external functions with more than 8 arguments are not supported");

    // build args
    DBGF("build args");
    std::vector<llvm::Value*> args;
    unsigned reg = XRegister::A0;
    unsigned i = 0;
    for (; i < numParams; i++) {
        llvm::Value* v = bld->load(reg++);
        llvm::Type* ty = ft->getParamType(i);

        // need to cast?
        if (ty != t.i32)
            v = bld->bitOrPointerCast(v, ty);

        args.push_back(v);
    }

    // varArgs: passing 4 extra args for now
    if (ft->isVarArg()) {
        unsigned n = MIN(i + 4, 8);
        for (; i < n; i++)
            args.push_back(bld->load(reg++));
    }

    // call the function
    llvm::Value* v;
    if (ft->getReturnType()->isVoidTy()) {
        v = bld->call(impf, args);
    } else {
        v = bld->call(impf, args, impf->getName());
        bld->store(v, XRegister::A0);
    }

    v = bld->retVoid();

    return addr;
}


llvm::Error Translator::genICaller()
{
    DBGF("");

    const Types& t = _ctx->t;
    Builder bldi(_ctx, NO_FIRST);
    Builder* bld = &bldi;
    llvm::Function* ic = _iCaller.func();
    xassert(ic);

    llvm::PointerType* ty = t.voidFunc->getPointerTo();
    xassert(!ic->arg_empty());
    llvm::Argument& arg = *ic->arg_begin();
    llvm::AllocaInst* target = nullptr;

    // basic blocks
    BasicBlock bbBeg(_ctx, "begin", ic);
    BasicBlock bbDfl(_ctx, "default", ic);
    BasicBlock bbEnd(_ctx, "end", ic);

    // switch
    bld->setInsertPoint(bbBeg);
    target = bld->_alloca(t.i32, _ctx->c.i32(1), "target");
    llvm::SwitchInst* sw = bld->sw(&arg, bbDfl, _funMap.size());

    // default: abort
    bld->setInsertPoint(bbDfl);
    bld->call(_sbtabort->func());
    bld->br(bbEnd);

    // end: call target
    bld->setInsertPoint(bbEnd);
    llvm::Value* v = bld->load(target);
    v = bld->intToPtr(v, ty);
    bld->call(v);
    bld->retVoid();

    // cases
    // case fun: arg = realFunAddress;
    for (const auto& p : _funcByAddr) {
        Function* f = p.val;
        uint64_t addr = f->addr();
        DBGF("function={0}, addr={1:X+8}", f->name(), addr);
        xassert(addr != Constants::INVALID_ADDR);

        std::string caseStr = "case_" + f->name();
        llvm::Value* sym = _ctx->module->getValueSymbolTable().lookup(f->name());

        BasicBlock dest(_ctx, caseStr, ic, bbDfl.bb());
        bld->setInsertPoint(dest);
        bld->store(bld->ptrToInt(sym, t.i32), target);
        bld->br(bbEnd);

        bld->setInsertPoint(bbBeg);
        sw->addCase(llvm::ConstantInt::get(t.i32, addr), dest.bb());
    }

    return llvm::Error::success();
}

} // sbt
