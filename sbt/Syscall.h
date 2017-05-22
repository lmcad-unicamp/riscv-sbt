#pragma once

#include "Context.h"

#include <llvm/Support/Error.h>

namespace llvm {
class Function;
class FunctionType;
class Module;
}

namespace sbt {

class Syscall
{
public:
  Syscall(Context* ctx) :
    _ctx(ctx)
  {}

  void call();

  void declHandler();

  llvm::Error genHandler();

private:
  Context* _ctx;
  Types& _t = _ctx->t;

  // riscv syscall function
  llvm::FunctionType* _ftRVSC;
  llvm::Function* _fRVSC;
};

}
