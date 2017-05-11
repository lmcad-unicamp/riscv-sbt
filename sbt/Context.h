#ifndef SBT_CONTEXT_H
#define SBT_CONTEXT_H

#include <llvm/IR/IRBuilder.h>

namespace sbt {

class Context
{
public:

  Context(
    llvm::LLVMContext* ctx,
    llvm::Module* mod,
    llvm::IRBuilder<>* bld)
    :
    ctx(ctx),
    module(mod),
    builder(bld)
  {}


  llvm::LLVMContext* ctx;
  llvm::Module* module;
  llvm::IRBuilder<>* builder;
};

}

#endif
