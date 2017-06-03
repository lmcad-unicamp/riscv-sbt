#include "Syscall.h"

#include "BasicBlock.h"
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
  llvm::Module* module = _ctx->module;
  const auto& c = _ctx->c;

  // declare X86 syscall functions
  const size_t n = 5;
  llvm::FunctionType* ftX86SC[n];
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

  // args #, RISC-V syscall #, x86 syscall #
  const std::vector<Syscall> scv = {
    { 1, 93, 1 },  // EXIT
    { 3, 64, 4 }   // WRITE
  };

  const int X86_SYS_EXIT = 1;
  const std::string bbPrefix = "bb_rvsc_";

  declHandler();

  Builder bldi(_ctx, NO_FIRST);
  Builder* bld = &bldi;

  // entry
  BasicBlock bbEntry(_ctx, bbPrefix + "entry", _fRVSC);
  llvm::Argument &sc = *_fRVSC->arg_begin();

  // exit
  //
  // return A0
  BasicBlock bbExit(_ctx, bbPrefix + "exit", _fRVSC);
  bld->setInsertPoint(bbExit);
  bld->ret(bld->load(XRegister::A0));

  // 2nd switch
  BasicBlock bbSW2(_ctx, bbPrefix + "sw2", _fRVSC, bbExit.bb());

  // first Switch:
  // - switch based on RISC-V syscall number and:
  //   - set number of args
  //   - set X86 syscall number

  // default case: call exit(99)
  BasicBlock bbSW1Dfl(_ctx, bbPrefix + "sw1_default", _fRVSC, bbSW2.bb());
  bld->setInsertPoint(bbSW1Dfl);
  bld->store(c.i32(1), XRegister::T0);
  bld->store(c.i32(X86_SYS_EXIT), XRegister::A7);
  bld->store(c.i32(99), XRegister::A0);
  bld->br(bbSW2);

  bld->setInsertPoint(bbEntry);
  llvm::SwitchInst *sw1 = bld->sw(&sc, bbSW1Dfl);

  // other cases
  auto addSW1Case = [&](const Syscall& s) {
    std::string sss = bbPrefix;
    llvm::raw_string_ostream ss(sss);
    ss << "sw1_case_" << s.rv;

    BasicBlock bb(_ctx, ss.str(), _fRVSC, bbSW2.bb());
    bld->setInsertPoint(bb);
    bld->store(llvm::ConstantInt::get(_t.i32, s.args), XRegister::T0);
    bld->store(llvm::ConstantInt::get(_t.i32, s.x86), XRegister::A7);
    bld->br(bbSW2);
    sw1->addCase(_ctx->c.i32(s.rv), bb.bb());
  };

  for (const Syscall& s : scv)
    addSW1Case(s);

  // second Switch:
  // - switch based on syscall's number of args
  //   and make the call

  auto getSW2CaseBB = [&](size_t val) {
    std::string sss = bbPrefix;
    llvm::raw_string_ostream ss(sss);
    ss << "sw2_case_" << val;

    BasicBlock bb(_ctx, ss.str(), _fRVSC, bbExit.bb());
    bld->setInsertPoint(bb);

    // set args
    std::vector<llvm::Value*> args = { bld->load(XRegister::A7) };
    for (size_t i = 0; i < val; i++)
      args.push_back(bld->load(XRegister::A0 + i));

    // Make the syscall
    llvm::Value* v = bld->call(fX86SC[val], args);
    bld->store(v, XRegister::A0);
    bld->br(bbExit);
    return bb;
  };

  BasicBlock sw2Case0 = getSW2CaseBB(0);

  // switch 2
  bld->setInsertPoint(bbSW2);
  llvm::SwitchInst* sw2 = bld->sw(bld->load(XRegister::T0), sw2Case0);
  sw2->addCase(_ctx->c.ZERO, sw2Case0.bb());
  for (size_t i = 1; i < n; i++)
    sw2->addCase(_ctx->c.i32(i), getSW2CaseBB(i).bb());

  return llvm::Error::success();
}


void Syscall::call()
{
  Builder* bld = _ctx->bld;

  llvm::Value* sc = bld->load(XRegister::A7);
  std::vector<llvm::Value*> args = { sc };
  llvm::Value* v = bld->call(_fRVSC, args);
  bld->store(v, XRegister::A0);
}

}
