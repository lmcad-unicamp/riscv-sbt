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
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>

namespace llvm {
Target& getTheRISCVMaster32Target();
}


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
  // logs() << "Triple: " << TripleName << "\n";

  _target = &llvm::getTheRISCVMaster32Target();
  // Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!_target) {
    serr << "Target not found: " << tripleName;
    return error(serr);
  }

  _mri.reset(_target->createMCRegInfo(tripleName));
  if (!_mri) {
    serr << "No register info for target " << tripleName;
    return error(serr);
  }

  // Set up disassembler.
  _asmInfo.reset(_target->createMCAsmInfo(*_mri, tripleName));
  if (!_asmInfo) {
    serr << "No assembly info for target " << tripleName;
    return error(serr);
  }

  llvm::SubtargetFeatures features;
  _sti.reset(
      _target->createMCSubtargetInfo(tripleName, "", features.getString()));
  if (!_sti) {
    serr << "No subtarget info for target " << tripleName;
    return error(serr);
  }

  _mofi.reset(new llvm::MCObjectFileInfo);
  _mc.reset(new llvm::MCContext(_asmInfo.get(), _mri.get(), _mofi.get()));
  _disAsm.reset(_target->createMCDisassembler(*_sti, *_mc));
  if (!_disAsm) {
    serr << "No disassembler for target " << tripleName;
    return error(serr);
  }

  _mii.reset(_target->createMCInstrInfo());
  if (!_mii) {
    serr << "No instruction info for target " << tripleName;
    return error(serr);
  }

  _instPrinter.reset(
    _target->createMCInstPrinter(triple, 0, *_asmInfo, *_mii, *_mri));
  if (!_instPrinter) {
    serr << "No instruction printer for target " << tripleName;
    return error(serr);
  }

  //
  //
  //
  _ctx->x = new XRegisters(_ctx, DECL);
  _ctx->stack = new Stack(_ctx);
  _ctx->disasm = new Disassembler(&*_disAsm, &*_instPrinter, &*_sti);
  _ctx->_func = &_funMap;
  _ctx->_funcByAddr = &_funcByAddr;

  _iCaller.create();

  // host functions

  llvm::FunctionType *ft = llvm::FunctionType::get(_ctx->t.i32, !VAR_ARG);

  _getCycles.reset(new Function(_ctx, "get_cycles"));
  _getTime.reset(new Function(_ctx, "get_time"));
  _getInstRet.reset(new Function(_ctx, "get_instret"));

  _getCycles->create(ft);
  _getTime->create(ft);
  _getInstRet->create(ft);

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
  std::string rv32Func = "rv32_" + func;

  auto builder = _ctx->builder;
  Builder bld(_ctx);
  auto& ctx = *_ctx->ctx;
  auto module = _ctx->module;
  auto& t = _ctx->t;

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

  // declare imported function in our module
  llvm::Function* impf =
    llvm::Function::Create(ft,
        llvm::GlobalValue::ExternalLinkage, func, module);

  // create our caller to the external function
  if (!_extFuncAddr)
    _extFuncAddr = 0xFFFF0000;
  uint64_t addr = _extFuncAddr;
  FunctionPtr f(new Function(_ctx, rv32Func, nullptr, addr));
  f->create(t.voidFunc, llvm::Function::PrivateLinkage);
  // add to maps
  _funcByAddr(_extFuncAddr, &*f);
  _funMap(f->name(), std::move(f));
  _extFuncAddr += Instruction::SIZE;

  llvm::BasicBlock* bb = llvm::BasicBlock::Create(ctx, "entry", f->func());
  llvm::BasicBlock* prevBB = builder->GetInsertBlock();
  builder->SetInsertPoint(bb);

  OnScopeExit RestoreInsertPoint(
    [builder, prevBB]() {
      builder->SetInsertPoint(prevBB);
    });

  unsigned numParams = ft->getNumParams();
  xassert(numParams < 9 &&
      "External functions with more than 8 arguments are not supported!");

  // build args
  std::vector<llvm::Value*> args;
  unsigned reg = XRegister::A0;
  unsigned i = 0;
  for (; i < numParams; i++) {
    llvm::Value* v = bld.load(reg++);
    llvm::Type* ty = ft->getParamType(i);

    // need to cast?
    if (ty != t.i32)
      v = builder->CreateBitOrPointerCast(v, ty);

    args.push_back(v);
  }

  // varArgs: passing 4 extra args for now
  if (ft->isVarArg()) {
    unsigned n = MIN(i + 4, 8);
    for (; i < n; i++)
      args.push_back(bld.load(reg++));
  }

  // call the Function
  llvm::Value* v;
  if (ft->getReturnType()->isVoidTy()) {
    v = builder->CreateCall(impf, args);
  } else {
    v = builder->CreateCall(impf, args, impf->getName());
    bld.store(v, XRegister::A0);
  }

  v = builder->CreateRetVoid();

  return addr;
}


llvm::Error Translator::genICaller()
{
  llvm::LLVMContext& ctx = *_ctx->ctx;
  const Constants& c = _ctx->c;
  const Types& t = _ctx->t;
  auto builder = _ctx->builder;
  Builder bld(_ctx);
  llvm::Function* ic = _iCaller.func();

  llvm::PointerType* ty = t.voidFunc->getPointerTo();
  llvm::Value* target = nullptr;

  // basic blocks
  llvm::BasicBlock* bbPrev = builder->GetInsertBlock();
  llvm::BasicBlock* bbBeg = llvm::BasicBlock::Create(ctx, "begin", ic);
  llvm::BasicBlock* bbDfl = llvm::BasicBlock::Create(ctx, "default", ic);
  llvm::BasicBlock* bbEnd = llvm::BasicBlock::Create(ctx, "end", ic);

  // default:
  // t1 = nullptr;
  builder->SetInsertPoint(bbDfl);
  bld.store(c.ZERO, XRegister::T1);
  builder->CreateBr(bbEnd);

  // end: call t1
  builder->SetInsertPoint(bbEnd);
  target = bld.load(XRegister::T1);
  target = builder->CreateIntToPtr(target, ty);
  builder->CreateCall(target);
  bld.retVoid();

  // switch
  builder->SetInsertPoint(bbBeg);
  target = bld.load(XRegister::T1);
  llvm::SwitchInst* sw = builder->CreateSwitch(target, bbDfl, _funMap.size());

  // cases
  // case fun: t1 = realFunAddress;
  for (const auto& p : _funMap) {
    const FunctionPtr& f = p.val;
    uint64_t addr = f->addr();
    xassert(addr && "invalid function address");

    std::string caseStr = "case_" + f->name();
    llvm::Value* sym = _ctx->module->getValueSymbolTable().lookup(f->name());

    llvm::BasicBlock* dest =
      llvm::BasicBlock::Create(ctx, caseStr, ic, bbDfl);
    builder->SetInsertPoint(dest);
    sym = builder->CreatePtrToInt(sym, t.i32);
    bld.store(sym, XRegister::T1);
    builder->CreateBr(bbEnd);

    builder->SetInsertPoint(bbBeg);
    sw->addCase(llvm::ConstantInt::get(t.i32, addr), dest);
  }

  builder->SetInsertPoint(bbPrev);
  return llvm::Error::success();
}

} // sbt
