#include "Function.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Constants.h"
#include "SBTError.h"
#include "Section.h"
#include "Stack.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormatVariadic.h>

namespace sbt {

llvm::Error Function::create(llvm::FunctionType* ft)
{
  _f = _ctx->module->getFunction(_name);
  if (_f)
    return llvm::Error::success();

  // create a function with no parameters
  if (!ft)
    ft = llvm::FunctionType::get(_ctx->t.voidT, !VAR_ARG);
  _f = llvm::Function::Create(ft,
    llvm::Function::ExternalLinkage, _name, _ctx->module);

  return llvm::Error::success();
}


llvm::Error Function::translate()
{
  DBGS << _name << ":\n";

  if (_name == "main") {
    if (auto err = startMain())
      return err;
  } else {
    if (auto err = start())
      return err;
  }

  if (auto err = translateInstrs(_addr, _end))
    return err;

  if (auto err = finish())
    return err;

  return llvm::Error::success();
}


llvm::Error Function::startMain()
{
  const Types& t = _ctx->t;
  auto builder = _ctx->builder;
  Builder bld(_ctx);

  // create a function with no parameters
  llvm::FunctionType *ft =
    llvm::FunctionType::get(t.i32, !VAR_ARG);
  _f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
    _name, _ctx->module);

  // bb
  BasicBlock bb(_ctx, _addr, _f);
  _bbMap(_addr, std::move(bb));
  bld.setInsertPoint(bb);

  // set stack pointer
  bld.store(_ctx->stack->end(), XRegister::SP);

  // if (auto E = startup())
  //  return E;

  // init syscall module
  llvm::Function* f = llvm::Function::Create(t.voidFunc,
    llvm::Function::ExternalLinkage, "syscall_init", _ctx->module);
  builder->CreateCall(f);

  return llvm::Error::success();
}


llvm::Error Function::start()
{
  if (auto err = create())
    return err;

  // FunMap(F, std::move(Addr));

  // BB
  BasicBlock *bb = _bbMap[_addr];
  if (!bb) {
    _bbMap(_addr, BasicBlock(_ctx, _addr, _f));
    bb = _bbMap[_addr];
  } else {
    auto b = bb->bb();
    b->removeFromParent();
    b->insertInto(_f);
  }
  Builder bld(_ctx);
  bld.setInsertPoint(*bb);

  return llvm::Error::success();
}


llvm::Error Function::finish()
{
  auto builder = _ctx->builder;
  if (builder->GetInsertBlock()->getTerminator() == nullptr)
    builder->CreateRetVoid();
  return llvm::Error::success();
}


llvm::Error Function::translateInstrs(uint64_t st, uint64_t end)
{
  DBGS << __FUNCTION__ << llvm::formatv("({0:X+4}, {1:X+4})\n", st, end);

  ConstSectionPtr section = _sec->section();
  const llvm::ArrayRef<uint8_t> bytes = _sec->bytes();

  // for each instruction
  uint64_t size = Instruction::SIZE;
  for (uint64_t addr = st; addr < end; addr += size) {
    // disasm
    const uint8_t* rawBytes = &bytes[addr];
    uint32_t rawInst = *reinterpret_cast<const uint32_t*>(rawBytes);

    // consider 0 bytes as end-of-section padding
    if (_state == ST_PADDING) {
      if (rawInst != 0) {
        SBTError serr;
        serr << "Found non-zero byte in zero-padding area";
        return error(serr);
      }
      continue;
    } else if (rawInst == 0) {
      _state = ST_PADDING;
      continue;
    }

    Instruction inst(_ctx, rawInst);
    if (auto err = inst.translate())
      return err;
  }

  return llvm::Error::success();
}


#if 0
llvm::Expected<uint64_t> Translator::import(llvm::StringRef Func)
{
  std::string RV32Func = "rv32_" + Func.str();

  // check if the function was already processed
  if (Function *F = Module->getFunction(RV32Func))
    return *FunMap[F];

  // Load LibC module
  if (!LCModule) {
    if (!LIBC_BC) {
      SBTError SE;
      SE << "libc.bc file not found";
      return error(SE);
    }

    auto Res = MemoryBuffer::getFile(*LIBC_BC);
    if (!Res)
      return errorCodeToError(Res.getError());
    MemoryBufferRef Buf = **Res;

    auto ExpMod = parseBitcodeFile(Buf, *Context);
    if (!ExpMod)
      return ExpMod.takeError();
    LCModule = std::move(*ExpMod);
  }

  // lookup function
  Function *LF = LCModule->getFunction(Func);
  if (!LF) {
    SBTError SE;
    SE << "Function not found: " << Func;
    return error(SE);
  }
  FunctionType *FT = LF->getFunctionType();

  // declare imported function in our module
  Function *IF =
    Function::Create(FT, GlobalValue::ExternalLinkage, Func, Module);

  // create our caller to the external function
  FunctionType *VFT = FunctionType::get(Builder->getVoidTy(), !VAR_ARG);
  Function *F =
    Function::Create(VFT, GlobalValue::PrivateLinkage, RV32Func, Module);

  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  BasicBlock *PrevBB = Builder->GetInsertBlock();
  Builder->SetInsertPoint(BB);

  OnScopeExit RestoreInsertPoint(
    [this, PrevBB]() {
      Builder->SetInsertPoint(PrevBB);
    });

  unsigned NumParams = FT->getNumParams();
  assert(NumParams < 9 &&
      "External functions with more than 8 arguments are not supported!");

  // build Args
  std::vector<Value *> Args;
  unsigned Reg = RV_A0;
  unsigned I = 0;
  for (; I < NumParams; I++) {
    Value *V = load(Reg++);
    Type *Ty = FT->getParamType(I);

    // need to cast?
    if (Ty != I32)
      V = Builder->CreateBitOrPointerCast(V, Ty);

    Args.push_back(V);
  }

  // VarArgs: passing 4 extra args for now
  if (FT->isVarArg()) {
    unsigned N = MIN(I + 4, 8);
    for (; I < N; I++)
      Args.push_back(load(Reg++));
  }

  // call the Function
  Value *V;
  if (FT->getReturnType()->isVoidTy()) {
    V = Builder->CreateCall(IF, Args);
    updateFirst(V);
  } else {
    V = Builder->CreateCall(IF, Args, IF->getName());
    updateFirst(V);
    store(V, RV_A0);
  }

  V = Builder->CreateRetVoid();
  updateFirst(V);

  if (!ExtFunAddr)
    ExtFunAddr = CurSection->size();
  FunTable.push_back(F);
  FunMap(F, std::move(ExtFunAddr));
  uint64_t Ret = ExtFunAddr;
  ExtFunAddr += 4;

  return Ret;
}
#endif

}
