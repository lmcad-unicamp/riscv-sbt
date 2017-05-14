#include "Function.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Constants.h"
#include "Stack.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

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



#if 0
llvm::Error Function::start(llvm::StringRef name, uint64_t addr)
{
  if (auto E = finish())
    return E;

  auto ExpF = create(name);
  if (!ExpF)
    return ExpF.takeError();
  llvm::Function *f = ExpF.get();
  CurFunc = F;
  FunTable.push_back(F);
  FunMap(F, std::move(Addr));

  // BB
  BasicBlock **BBPtr = BBMap[Addr];
  BasicBlock *BB;
  if (!BBPtr) {
    BB = SBTBasicBlock::create(*Context, Addr, F);
    BBMap(Addr, std::move(BB));
  } else {
    BB = *BBPtr;
    BB->removeFromParent();
    BB->insertInto(F);
  }
  Builder->SetInsertPoint(BB);

  return Error::success();
}

llvm::Error Function::finish()
{
  if (!CurFunc)
    return Error::success();
  CurFunc = nullptr;

  if (Builder->GetInsertBlock()->getTerminator() == nullptr)
    Builder->CreateRetVoid();
  InMain = false;
  return Error::success();
}


llvm::Error Translator::translateInstrs(uint64_t Begin, uint64_t End)
{
  DBGS << __FUNCTION__ << formatv("({0:X+4}, {1:X+4})\n", Begin, End);

  uint64_t SectionAddr = CurSection->address();

  // for each instruction
  uint64_t Size;
  for (uint64_t Addr = Begin; Addr < End; Addr += Size) {
    setCurAddr(Addr);

    // disasm
    const uint8_t* RawBytes = &CurSectionBytes[Addr];
    uint32_t RawInst = *reinterpret_cast<const uint32_t*>(RawBytes);

    // consider 0 bytes as end-of-section padding
    if (State == ST_PADDING) {
      if (RawInst != 0) {
        SBTError SE;
        SE << "Found non-zero byte in zero-padding area";
        return error(SE);
      }
      continue;
    } else if (RawInst == 0) {
      State = ST_PADDING;
      continue;
    }

    MCInst Inst;
    MCDisassembler::DecodeStatus st =
      DisAsm->getInstruction(Inst, Size,
        CurSectionBytes.slice(Addr),
        SectionAddr + Addr, DBGS, nulls());
    if (st == MCDisassembler::DecodeStatus::Success) {
#if SBT_DEBUG
      DBGS << llvm::formatv("{0:X-4}: ", Addr);
      InstPrinter->printInst(&Inst, DBGS, "", *STI);
      DBGS << "\n";
#endif
      // translate
      if (auto E = translate(Inst))
        return E;
    // failed to disasm
    } else {
      SBTError SE;
      SE << "Invalid instruction encoding at address ";
      SE << formatv("{0:X-4}", Addr);
      SE << formatv(": {0:X-8}", RawInst);
      return error(SE);
    }
  }

  return Error::success();
}

Error Translator::startMain(StringRef Name, uint64_t Addr)
{
  if (auto E = finishFunction())
    return E;

  // Create a function with no parameters
  FunctionType *FT =
    FunctionType::get(I32, !VAR_ARG);
  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, Module);
  CurFunc = F;

  // BB
  BasicBlock *BB = SBTBasicBlock::create(*Context, Addr, F);
  BBMap(Addr, std::move(BB));
  Builder->SetInsertPoint(BB);

  // Set stack pointer.

  std::vector<Value *> Idx = { ZERO, ConstantInt::get(I32, StackSize) };
  Value *V =
    Builder->CreateGEP(Stack, Idx);
  StackEnd = i8PtrToI32(V);

  store(StackEnd, RV_SP);

  // if (auto E = startup())
  //  return E;

  // init syscall module
  F = Function::Create(VoidFun,
    Function::ExternalLinkage, "syscall_init", Module);
  Builder->CreateCall(F);

  InMain = true;

  return Error::success();
}


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
