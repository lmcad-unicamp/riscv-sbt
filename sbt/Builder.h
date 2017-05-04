#ifndef SBT_BUILDER_H
#define SBT_BUILDER_H

#include "Constants.h"
#include "Register.h"

#include <llvm/IR/IRBuilder.h>

namespace sbt {

class Builder
{
public:
  Builder(llvm::IRBuilder<>* builder) :
    _builder(builder)
  {}

  // load register
  llvm::LoadInst* load(unsigned reg)
  {
    llvm::LoadInst* i = _builder->CreateLoad(
        Register::rvX[reg], Register::rvX[reg]->getName() + "_");
    updateFirst(i);
    return i;
  }

  // store value to register
  llvm::StoreInst* store(llvm::Value* v, unsigned reg)
  {
    if (reg == 0)
      return nullptr;

    llvm::StoreInst* i = _builder->CreateStore(v,
        Register::rvX[reg], !VOLATILE);
    updateFirst(i);
    return i;
  }

  // add
  llvm::Value* add(llvm::Value* a, llvm::Value* b)
  {
    llvm::Value* v = _builder->CreateAdd(a, b);
    updateFirst(v);
    return v;
  }

  // nop
  void nop()
  {
    load(Register::RV_ZERO);
  }

  llvm::Value *i8PtrToI32(llvm::Value* v8)
  {
    // cast to int32_t *
    llvm::Value* v =
      _builder->CreateCast(llvm::Instruction::CastOps::BitCast, v8, I32Ptr);
    updateFirst(v);
    // Cast to int32_t
    v = _builder->CreateCast(llvm::Instruction::CastOps::PtrToInt, v, I32);
    updateFirst(v);
    return v;
  }

private:
  llvm::IRBuilder<>* _builder;
  llvm::Value* _first = nullptr;

  void updateFirst(llvm::Value* v)
  {
    if (!_first)
      _first = llvm::dyn_cast<llvm::Instruction>(v);
  }
};

}

#endif
