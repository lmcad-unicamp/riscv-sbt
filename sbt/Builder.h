#ifndef SBT_BUILDER_H
#define SBT_BUILDER_H

#include "Context.h"
#include "Register.h"

namespace sbt {

class Builder
{
  Context* _ctx;
  llvm::IRBuilder<>* _builder;
  llvm::Value* _first = nullptr;

public:
  Builder(Context* ctx) :
    _ctx(ctx),
    _builder(ctx->builder)
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
      _builder->CreateCast(llvm::Instruction::CastOps::BitCast, v8, _ctx->t.i32ptr);
    updateFirst(v);
    // Cast to int32_t
    v = _builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
      v, _ctx->t.i32);
    updateFirst(v);
    return v;
  }

  llvm::Instruction* retVoid()
  {
    auto v = _builder->CreateRetVoid();
    updateFirst(v);
    return v;
  }

private:
  void updateFirst(llvm::Value* v)
  {
    if (!_first)
      _first = llvm::dyn_cast<llvm::Instruction>(v);
  }
};

}

#endif
