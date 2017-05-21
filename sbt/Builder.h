#ifndef SBT_BUILDER_H
#define SBT_BUILDER_H

#include "BasicBlock.h"
#include "Context.h"
#include "Types.h"
#include "XRegisters.h"

namespace sbt {

class Builder
{
  Context* _ctx;
  llvm::IRBuilder<>* _builder;
  llvm::Instruction* _first = nullptr;
  Types* _t;

public:
  Builder(Context* ctx) :
    _ctx(ctx),
    _builder(ctx->builder),
    _t(&_ctx->t)
  {}

  ~Builder()
  {
    if (!_first)
      _first = load(XRegister::A0);
  }

  void reset()
  {
    _first = nullptr;
  }

  // load register
  llvm::LoadInst* load(unsigned reg)
  {
    const XRegister& x = _ctx->x[reg];
    llvm::LoadInst* i = _builder->CreateLoad(
        x.var(), x.name() + "_");
    updateFirst(i);
    return i;
  }

  // store value to register
  llvm::StoreInst* store(llvm::Value* v, unsigned reg)
  {
    if (reg == 0)
      return nullptr;

    const XRegister& x = _ctx->x[reg];
    llvm::StoreInst* i = _builder->CreateStore(v,
        x.var(), !VOLATILE);
    updateFirst(i);
    return i;
  }

  // ALU ops

  llvm::Value* add(llvm::Value* a, llvm::Value* b)
  {
    llvm::Value* v = _builder->CreateAdd(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* _and(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateAnd(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* mul(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateMul(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* _or(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateOr(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* sll(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateShl(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* ult(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpULT(a, b);
    updateFirst(v);
    _builder->CreateZExt(v, _t->i32);
    return v;
  }

  llvm::Value* slt(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpSLT(a, b);
    updateFirst(v);
    _builder->CreateZExt(v, _t->i32);
    return v;
  }

  llvm::Value* sra(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateAShr(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* srl(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateLShr(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* sub(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateSub(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* _xor(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateXor(a, b);
    updateFirst(v);
    return v;
  }


  // nop
  void nop()
  {
    load(XRegister::ZERO);
  }

  // i8* to i32
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

  // gep
  llvm::Value* gep(llvm::Value* ptr, std::vector<llvm::Value*> idx)
  {
    llvm::Value* v = _builder->CreateGEP(ptr, idx);
    updateFirst(v);
    return v;
  }

  llvm::Instruction* retVoid()
  {
    auto v = _builder->CreateRetVoid();
    updateFirst(v);
    return v;
  }

  void setInsertPoint(BasicBlock& bb)
  {
    _builder->SetInsertPoint(bb.bb());
  }

  llvm::Instruction* first()
  {
    return _first;
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
