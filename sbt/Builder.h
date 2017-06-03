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
  Types* _t;
  llvm::Instruction* _first = nullptr;

public:
  Builder(Context* ctx) :
    _ctx(ctx),
    _builder(ctx->builder),
    _t(&_ctx->t)
  {}

  // dtor
  ~Builder()
  {
    if (!_first)
      nop();
  }

  /**
   * Reset builder, to start translating a new guest instruction.
   */
  void reset()
  {
    _first = nullptr;
  }

  // load
  llvm::LoadInst* load(llvm::Value* ptr)
  {
    llvm::LoadInst* v = _builder->CreateLoad(ptr);
    updateFirst(v);
    return v;
  }

  // load register
  llvm::LoadInst* load(unsigned reg)
  {
    xassert(_ctx->x.get() && "XRegisters pointer is null!");
    const XRegister& x = _ctx->x[reg];
    llvm::LoadInst* i = _builder->CreateLoad(
        x.var(), x.name() + "_");
    updateFirst(i);
    return i;
  }

  // store
  llvm::StoreInst* store(llvm::Value* v, llvm::Value* ptr)
  {
    llvm::StoreInst* i = _builder->CreateStore(v, ptr);
    updateFirst(i);
    return i;
  }

  // store value in register
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

  // nop
  void nop()
  {
    load(XRegister::ZERO);
  }

  // sign extend
  llvm::Value* sext(llvm::Value* v)
  {
    llvm::Value* v2 = _builder->CreateSExt(v, _t->i32);
    updateFirst(v2);
    return v2;
  }

  // zero extend
  llvm::Value* zext(llvm::Value* v)
  {
    llvm::Value* v2 = _builder->CreateZExt(v, _t->i32);
    updateFirst(v2);
    return v2;
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


  // comparisons

  llvm::Value* eq(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpEQ(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* ne(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpNE(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* ult(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpULT(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* slt(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpSLT(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* sge(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpSGE(a, b);
    updateFirst(v);
    return v;
  }

  llvm::Value* uge(llvm::Value* a, llvm::Value* b) {
    llvm::Value* v = _builder->CreateICmpUGE(a, b);
    updateFirst(v);
    return v;
  }


  // casts

  // i8* to i32
  llvm::Value* i8PtrToI32(llvm::Value* v8)
  {
    // cast to int32*
    llvm::Value* v =
      _builder->CreateCast(llvm::Instruction::CastOps::BitCast,
        v8, _ctx->t.i32ptr);
    updateFirst(v);
    // cast to int32
    v = _builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
      v, _ctx->t.i32);
    updateFirst(v);
    return v;
  }

  // i32 to i8*
  llvm::Value* i32ToI8Ptr(llvm::Value* i32)
  {
    llvm::Value* v = _builder->CreateCast(
      llvm::Instruction::CastOps::IntToPtr, i32, _t->i8ptr);
    updateFirst(v);
    return v;
  }

  // i32 to i16*
  llvm::Value* i32ToI16Ptr(llvm::Value* i32)
  {
    llvm::Value* v = _builder->CreateCast(
      llvm::Instruction::CastOps::IntToPtr, i32, _t->i16ptr);
    updateFirst(v);
    return v;
  }

  // i32 to i32*
  llvm::Value* i32ToI32Ptr(llvm::Value* i32)
  {
    llvm::Value* v = _builder->CreateCast(
      llvm::Instruction::CastOps::IntToPtr, i32, _t->i32ptr);
    updateFirst(v);
    return v;
  }

  // int to ptr
  llvm::Value* intToPtr(llvm::Value* i, llvm::Type* t)
  {
    llvm::Value* v = _builder->CreateIntToPtr(i, t);
    updateFirst(v);
    return v;
  }

  // ptr to int
  llvm::Value* ptrToInt(llvm::Value* p, llvm::Type* t)
  {
    llvm::Value* v = _builder->CreatePtrToInt(p, t);
    updateFirst(v);
    return v;
  }


  // trunc or cast to i8
  llvm::Value* truncOrBitCastI8(llvm::Value* i32)
  {
    llvm::Value* v = _builder->CreateTruncOrBitCast(i32, _t->i8);
    updateFirst(v);
    return v;
  }

  // trunc or cast to i16
  llvm::Value* truncOrBitCastI16(llvm::Value* i32)
  {
    llvm::Value* v = _builder->CreateTruncOrBitCast(i32, _t->i16);
    updateFirst(v);
    return v;
  }

  // bit or pointer cast
  llvm::Value* bitOrPointerCast(llvm::Value* v, llvm::Type* ty)
  {
    llvm::Value* v2 = _builder->CreateBitOrPointerCast(v, ty);
    updateFirst(v2);
    return v2;
  }

  // gep (get element pointer)
  llvm::Value* gep(llvm::Value* ptr, std::vector<llvm::Value*> idx)
  {
    llvm::Value* v = _builder->CreateGEP(ptr, idx);
    updateFirst(v);
    return v;
  }

  // call function
  llvm::Value* call(
    llvm::Function* f,
    llvm::ArrayRef<llvm::Value*> args = llvm::None,
    llvm::StringRef vname = "")
  {
    llvm::Value* v = _builder->CreateCall(f, args, vname);
    updateFirst(v);
    return v;
  }

  // call function from pointer
  llvm::Value* call(
    llvm::Value* ptr,
    llvm::ArrayRef<llvm::Value*> args = llvm::None)
  {
    llvm::Value* v = _builder->CreateCall(ptr, args);
    updateFirst(v);
    return v;
  }

  // ret void
  llvm::Instruction* retVoid()
  {
    auto v = _builder->CreateRetVoid();
    updateFirst(v);
    return v;
  }

  // ret
  llvm::Value* ret(llvm::Value* r)
  {
    auto v = _builder->CreateRet(r);
    updateFirst(v);
    return v;
  }

  // br
  llvm::Value* br(const BasicBlock& bb)
  {
    llvm::Value* v = _builder->CreateBr(bb.bb());
    updateFirst(v);
    return v;
  }

  llvm::Value* br(BasicBlock* bb)
  {
    return br(*bb);
  }

  // condBr
  llvm::Value* condBr(llvm::Value* cond, BasicBlock* t, BasicBlock* f)
  {
    llvm::Value* v = _builder->CreateCondBr(cond, t->bb(), f->bb());
    updateFirst(v);
    return v;
  }

  // fence
  void fence(llvm::AtomicOrdering order, llvm::SynchronizationScope scope)
  {
    llvm::Value* v = _builder->CreateFence(order, scope);
    updateFirst(v);
  }

  // get insert basic block
  BasicBlock getInsertBlock()
  {
    return BasicBlock(_ctx, _builder->GetInsertBlock());
  }

  // set insert basic block
  void setInsertPoint(const BasicBlock& bb)
  {
    _builder->SetInsertPoint(bb.bb());
  }

  // switch
  llvm::SwitchInst* sw(llvm::Value* v, const BasicBlock& bb,
    unsigned numCases = 10)
  {
    llvm::SwitchInst* i = _builder->CreateSwitch(v, bb.bb(), numCases);
    updateFirst(i);
    return i;
  }

  // get first instruction produced by the builder since last reset
  llvm::Instruction* first()
  {
    return _first;
  }

private:

  // update first instruction pointer
  void updateFirst(llvm::Value* v)
  {
    if (!_first)
      _first = llvm::dyn_cast<llvm::Instruction>(v);
  }
};

}

#endif
