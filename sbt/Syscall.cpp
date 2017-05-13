#include "Syscall.h"

#include "Builder.h"
#include "Constants.h"

#include <llvm/IR/Function.h>

namespace sbt {

void Syscall::declHandler()
{
  _ftRVSC = llvm::FunctionType::get(_t.i32, { _t.i32 }, !VAR_ARG);
  _fRVSC = llvm::Function::Create(
      _ftRVSC, llvm::Function::ExternalLinkage, "rv_syscall",
      _ctx->module);
}


llvm::Error Syscall::genHandler()
{
  llvm::LLVMContext* ctx = _ctx->ctx;
  llvm::IRBuilder<>* builder = _ctx->builder;
  llvm::Module* module = _ctx->module;

  // declare X86 syscall functions
  const size_t n = 5;
  llvm::FunctionType *ftX86SC[n];
  llvm::Function* fX86SC[n];
  std::vector<llvm::Type*> fArgs = { _t.i32 };

  const std::string scName = "syscall";
  for (size_t i = 0; i < n; i++) {
    std::string s = scName;
    llvm::raw_string_ostream ss(s);
    ss << i;

    ftX86SC[i] = llvm::FunctionType::get(_t.i32, fArgs, !VAR_ARG);
    fArgs.push_back(_t.i32);

    fX86SC[i] = llvm::Function::Create(ftX86SC[i],
      llvm::Function::ExternalLinkage, ss.str(), module);
  }

  // build syscalls' info
  struct Syscall {
    int args;
    int rv;
    int x86;

    Syscall(int a, int r, int x) :
      args(a),
      rv(r),
      x86(x)
    {}
  };

  // Arg #, RISC-V Syscall #, X86 Syscall #
  const int X86_SYS_EXIT = 1;
  const std::vector<Syscall> scv = {
    { 1, 93, 1 },  // EXIT
    { 3, 64, 4 }   // WRITE
  };

  const std::string bbPrefix = "bb_rvsc_";

  declHandler();
  Builder bld(_ctx);

  // entry
  llvm::BasicBlock *bbEntry = llvm::BasicBlock::Create(
    *ctx, bbPrefix + "entry", _fRVSC);
  llvm::Argument &sc = *_fRVSC->arg_begin();

  // exit
  //
  // return A0
  llvm::BasicBlock *bbExit = llvm::BasicBlock::Create(*ctx,
    bbPrefix + "exit", _fRVSC);
  builder->SetInsertPoint(bbExit);
  builder->CreateRet(bld.load(XRegister::A0));

  // 2nd switch
  llvm::BasicBlock* bbSW2 = llvm::BasicBlock::Create(*ctx,
    bbPrefix + "sw2", _fRVSC, bbExit);

  // First Switch:
  // - switch based on RISC-V syscall number and:
  //   - set number of args
  //   - set X86 syscall number

  // Default case: call exit(99)
  llvm::BasicBlock* bbSW1Dfl = llvm::BasicBlock::Create(*ctx,
    bbPrefix + "sw1_default", _fRVSC, bbSW2);
  builder->SetInsertPoint(bbSW1Dfl);
  bld.store(llvm::ConstantInt::get(_t.i32, 1), XRegister::T0);
  bld.store(llvm::ConstantInt::get(_t.i32, X86_SYS_EXIT), XRegister::A7);
  bld.store(llvm::ConstantInt::get(_t.i32, 99), XRegister::A0);
  builder->CreateBr(bbSW2);

  builder->SetInsertPoint(bbEntry);
  llvm::SwitchInst *sw1 = builder->CreateSwitch(&sc, bbSW1Dfl);

  // Other cases
  auto addSW1Case = [&](const Syscall& s) {
    std::string sss = bbPrefix;
    llvm::raw_string_ostream ss(sss);
    ss << "sw1_case_" << s.rv;

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(
      *ctx, ss.str(), _fRVSC, bbSW2);
    builder->SetInsertPoint(bb);
    bld.store(llvm::ConstantInt::get(_t.i32, s.args), XRegister::T0);
    bld.store(llvm::ConstantInt::get(_t.i32, s.x86), XRegister::A7);
    builder->CreateBr(bbSW2);
    sw1->addCase(llvm::ConstantInt::get(_t.i32, s.rv), bb);
  };

  for (const Syscall& s : scv)
    addSW1Case(s);

  // Second Switch:
  // - switch based on syscall's number of args
  //   and make the call

  auto getSW2CaseBB = [&](size_t val) {
    std::string sss = bbPrefix;
    llvm::raw_string_ostream ss(sss);
    ss << "sw2_case_" << val;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(
      *ctx, ss.str(), _fRVSC, bbExit);
    builder->SetInsertPoint(bb);

    // Set args
    std::vector<llvm::Value*> args = { bld.load(XRegister::A7) };
    for (size_t i = 0; i < val; i++)
      args.push_back(bld.load(XRegister::A0 + i));

    // Make the syscall
    llvm::Value* v = builder->CreateCall(fX86SC[val], args);
    bld.store(v, XRegister::A0);
    builder->CreateBr(bbExit);
    return bb;
  };

  llvm::BasicBlock* sw2Case0 = getSW2CaseBB(0);

  builder->SetInsertPoint(bbSW2);
  llvm::SwitchInst* sw2 = builder->CreateSwitch(
    bld.load(XRegister::T0), sw2Case0);
  sw2->addCase(_ctx->c.ZERO, sw2Case0);
  for (size_t i = 1; i < n; i++)
    sw2->addCase(llvm::ConstantInt::get(_t.i32, i), getSW2CaseBB(i));

  return llvm::Error::success();
}

}
