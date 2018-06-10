#ifndef SBT_BUILDER_H
#define SBT_BUILDER_H

#include "BasicBlock.h"
#include "Context.h"
#include "Function.h"
#include "Types.h"
#include "XRegister.h"

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

class Builder
{
    Context* _ctx;
    llvm::IRBuilder<>* _builder;
    Types* _t;
    Constants* _c;
    llvm::Instruction* _first = nullptr;
    BasicBlock* _bb = nullptr;
    BasicBlock* _savedBB = nullptr;

public:
    /**
     * ctor.
     *
     * @param ctx
     * @param noFirst flag that indicates that we are not interested in the
     *                first instruction of this builder block.
     */
    Builder(Context* ctx, bool noFirst = false)
        :
        _ctx(ctx),
        _builder(ctx->builder),
        _t(&_ctx->t),
        _c(&_ctx->c)
    {
        if (noFirst)
            // just set it to something different than null
            _first = reinterpret_cast<llvm::Instruction*>(1);
    }

    // dtor
    ~Builder()
    {
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
        xassert(_ctx->func);
        Register& x = _ctx->func->getReg(reg);
        DBGF("reg={0}", x.name());
        llvm::LoadInst* i = _builder->CreateLoad(
                x.getForRead(), x.name() + "_");
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
        if (reg == XRegister::ZERO)
            return nullptr;

        xassert(_ctx->func);

        Register& x = _ctx->func->getReg(reg);
        llvm::StoreInst* i = _builder->CreateStore(v,
                x.getForWrite(), !VOLATILE);
        updateFirst(i);
        return i;
    }

    // nop
    void nop()
    {
        callIntrinsic(llvm::Intrinsic::donothing, {});
    }

    // sign extend
    llvm::Value* sext(llvm::Value* v)
    {
        llvm::Value* v2 = _builder->CreateSExt(v, _t->i32);
        updateFirst(v2);
        return v2;
    }

    llvm::Value* sext64(llvm::Value* v)
    {
        llvm::Value* v2 = _builder->CreateSExt(v, _t->i64);
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

    llvm::Value* zext64(llvm::Value* v)
    {
        llvm::Value* v2 = _builder->CreateZExt(v, _t->i64);
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

    llvm::Value* mulh(llvm::Value* a, llvm::Value* b) {
        llvm::Value* a64 = sext64(a);
        llvm::Value* b64 = sext64(b);
        llvm::Value* v = mul(a64, b64);
        v = sra(v, _c->i64(32));
        v = truncOrBitCastI32(v);
        return v;
    }

    llvm::Value* mulhu(llvm::Value* a, llvm::Value* b) {
        llvm::Value* a64 = zext64(a);
        llvm::Value* b64 = zext64(b);
        llvm::Value* v = mul(a64, b64);
        v = srl(v, _c->i64(32));
        v = truncOrBitCastI32(v);
        return v;
    }

    llvm::Value* mulhsu(llvm::Value* a, llvm::Value* b) {
        llvm::Value* a64 = sext64(a);
        llvm::Value* b64 = zext64(b);
        llvm::Value* v = mul(a64, b64);
        v = sra(v, _c->i64(32));
        v = truncOrBitCastI32(v);
        return v;
    }

    llvm::Value* div(llvm::Value* a, llvm::Value* b) {
        llvm::Value* v = _builder->CreateSDiv(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* divu(llvm::Value* a, llvm::Value* b) {
        llvm::Value* v = _builder->CreateUDiv(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* rem(llvm::Value* a, llvm::Value* b) {
        llvm::Value* v = _builder->CreateSRem(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* remu(llvm::Value* a, llvm::Value* b) {
        llvm::Value* v = _builder->CreateURem(a, b);
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

    llvm::Value* _not(llvm::Value* a) {
        return _xor(a, _ctx->c.i32(~0));
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

    // trunc or cast to i32
    llvm::Value* truncOrBitCastI32(llvm::Value* i64)
    {
        llvm::Value* v = _builder->CreateTruncOrBitCast(i64, _t->i32);
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
    llvm::ReturnInst* retVoid()
    {
        llvm::ReturnInst* v = _builder->CreateRetVoid();
        updateFirst(v);
        return v;
    }

    // ret
    llvm::ReturnInst* ret(llvm::Value* r)
    {
        llvm::ReturnInst* v = _builder->CreateRet(r);
        updateFirst(v);
        return v;
    }

    // br
    llvm::Value* br(const BasicBlock& bb)
    {
        DBGF("@{0}: {1}", _bb->name(), bb.name());
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
        DBGF("@{0}: {1}, {2}", _bb->name(), t->name(), f->name());
        llvm::Value* v = _builder->CreateCondBr(cond, t->bb(), f->bb());
        updateFirst(v);
        return v;
    }

    llvm::Value* indBr(llvm::Value* addr)
    {
        addr = bitOrPointerCast(addr, _t->i32ptr);
        llvm::Value* v = _builder->CreateIndirectBr(addr);
        updateFirst(v);
        return v;
    }

    // fence
    void fence(llvm::AtomicOrdering order, llvm::SyncScope::ID scope)
    {
        llvm::Value* v = _builder->CreateFence(order, scope);
        updateFirst(v);
    }

    // alloca
    llvm::AllocaInst* _alloca(
        llvm::Type* ty,
        llvm::Value* arrSize,
        const llvm::Twine& name)
    {
        llvm::AllocaInst* a = _builder->CreateAlloca(ty, arrSize, name);
        updateFirst(a);
        return a;
    }

    // get insert basic block
    BasicBlock* getInsertBlock()
    {
        xassert(_bb && "insert block not set!");
        return _bb;
    }

    // set insert basic block
    void setInsertBlock(BasicBlock* bb)
    {
        DBGF("{0}", bb->name());
        _bb = bb;
        _builder->SetInsertPoint(_bb->bb());
    }

    void saveInsertBlock()
    {
        xassert(_bb);
        _savedBB = _bb;
    }

    void restoreInsertBlock()
    {
        xassert(_savedBB);
        setInsertBlock(_savedBB);
        _savedBB = nullptr;
    }

    // switch
    llvm::SwitchInst* sw(llvm::Value* v, const BasicBlock& bb,
        unsigned numCases = 10)
    {
        llvm::SwitchInst* i = _builder->CreateSwitch(v, bb.bb(), numCases);
        updateFirst(i);
        return i;
    }

    // unreachable
    llvm::UnreachableInst* unreachable()
    {
        llvm::UnreachableInst* inst = _builder->CreateUnreachable();
        updateFirst(inst);
        return inst;
    }

    // get first instruction produced by the builder since last reset
    llvm::Instruction* first()
    {
        // create a nop if no instructions were generated by this builder
        // block
        if (!_first)
            nop();
        return _first;
    }

    // floating point ops

    // i32 to fp32*
    llvm::Value* i32ToFP32Ptr(llvm::Value* i32)
    {
        llvm::Value* v = _builder->CreateCast(
            llvm::Instruction::CastOps::IntToPtr, i32, _t->fp32ptr);
        updateFirst(v);
        return v;
    }

    // i32 to fp64*
    llvm::Value* i32ToFP64Ptr(llvm::Value* i32)
    {
        llvm::Value* v = _builder->CreateCast(
            llvm::Instruction::CastOps::IntToPtr, i32, _t->fp64ptr);
        updateFirst(v);
        return v;
    }

    // fp64* to fp32*
    llvm::Value* fp64PtrToFP32Ptr(llvm::Value* f64)
    {
        llvm::Value* v = _builder->CreateBitCast(
            f64, _t->fp32ptr);
        updateFirst(v);
        return v;
    }

    // fp32* to fp64*
    llvm::Value* fp32PtrToFP64Ptr(llvm::Value* f32)
    {
        llvm::Value* v = _builder->CreateBitCast(
            f32, _t->fp64ptr);
        updateFirst(v);
        return v;
    }

    // fp32 to fp64
    llvm::Value* fp32ToFP64(llvm::Value* fp32)
    {
        llvm::Value* v = _builder->CreateFPCast(fp32, _t->fp64);
        updateFirst(v);
        return v;
    }

    // fp64 to fp32
    llvm::Value* fp64ToFP32(llvm::Value* fp64)
    {
        llvm::Value* v = _builder->CreateFPCast(fp64, _t->fp32);
        updateFirst(v);
        return v;
    }

    // integer to FP
    llvm::Value* siToFP(llvm::Value* i, llvm::Type* ty)
    {
        llvm::Value* v =
            _builder->CreateCast(llvm::Instruction::CastOps::SIToFP, i, ty);
        updateFirst(v);
        return v;
    }

    // FP to integer
    llvm::Value* fpToSI(llvm::Value* fp, llvm::Type* ty)
    {
        llvm::Value* v =
            _builder->CreateCast(llvm::Instruction::CastOps::FPToSI, fp, ty);
        updateFirst(v);
        return v;
    }

    // unsigned integer to FP
    llvm::Value* uiToFP(llvm::Value* ui, llvm::Type* ty)
    {
        llvm::Value* v =
            _builder->CreateCast(llvm::Instruction::CastOps::UIToFP, ui, ty);
        updateFirst(v);
        return v;
    }

    // FP to unsigned integer
    llvm::Value* fpToUI(llvm::Value* fp, llvm::Type* ty)
    {
        llvm::Value* v =
            _builder->CreateCast(llvm::Instruction::CastOps::FPToUI, fp, ty);
        updateFirst(v);
        return v;
    }

    // load f register

    llvm::LoadInst* fload32(unsigned reg)
    {
        xassert(_ctx->func);
        Register& f = _ctx->func->getFReg(reg);
        DBGF("reg={0}", f.name());
        llvm::Value* ptr = f.getForRead();
        ptr = fp64PtrToFP32Ptr(ptr);
        llvm::LoadInst* i = _builder->CreateLoad(ptr, f.name() + "_");
        updateFirst(i);
        return i;
    }

    llvm::LoadInst* fload64(unsigned reg)
    {
        xassert(_ctx->func);
        Register& f = _ctx->func->getFReg(reg);
        DBGF("reg={0}", f.name());
        llvm::LoadInst* i = _builder->CreateLoad(
                f.getForRead(), f.name() + "_");
        updateFirst(i);
        return i;
    }

    // store value in f register

    llvm::StoreInst* fstore32(llvm::Value* v, unsigned reg)
    {
        xassert(_ctx->func);

        Register& f = _ctx->func->getFReg(reg);
        llvm::Value* ptr = f.getForWrite();
        ptr = fp64PtrToFP32Ptr(ptr);
        llvm::StoreInst* i = _builder->CreateStore(v, ptr, !VOLATILE);
        updateFirst(i);
        return i;
    }

    llvm::StoreInst* fstore64(llvm::Value* v, unsigned reg)
    {
        xassert(_ctx->func);

        Register& f = _ctx->func->getFReg(reg);
        llvm::StoreInst* i = _builder->CreateStore(v,
                f.getForWrite(), !VOLATILE);
        updateFirst(i);
        return i;
    }

    // FPU ops

    llvm::Value* fadd(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFAdd(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* fsub(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFSub(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* fmul(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFMul(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* fdiv(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFDiv(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* fmin(llvm::Value* a, llvm::Value* b)
    {
        return callIntrinsic(llvm::Intrinsic::minnum, {a, b});
    }

    llvm::Value* fmax(llvm::Value* a, llvm::Value* b)
    {
        return callIntrinsic(llvm::Intrinsic::maxnum, {a, b});
    }

    llvm::Value* fsqrt(llvm::Value* a)
    {
        return callIntrinsic(llvm::Intrinsic::sqrt, {a});
    }

    llvm::Value* fmadd(llvm::Value* a, llvm::Value* b, llvm::Value* c)
    {
        return callIntrinsic(llvm::Intrinsic::fmuladd, {a, b, c});
    }

    llvm::Value* fmsub(llvm::Value* a, llvm::Value* b, llvm::Value* c)
    {
        return fsub(fadd(a, b), c);
    }

    llvm::Value* fneg(llvm::Value* a)
    {
        llvm::Value* v = _builder->CreateFNeg(a);
        updateFirst(v);
        return v;
    }

    llvm::Value* fnmadd(llvm::Value* a, llvm::Value* b, llvm::Value* c)
    {
        return fneg(fmadd(a, b, c));
    }

    llvm::Value* fnmsub(llvm::Value* a, llvm::Value* b, llvm::Value* c)
    {
        return fneg(fmsub(a, b, c));
    }

    llvm::Value* fabs(llvm::Value* a)
    {
        return callIntrinsic(llvm::Intrinsic::fabs, {a});
    }

    llvm::Value* feq(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFCmpOEQ(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* fle(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFCmpOLE(a, b);
        updateFirst(v);
        return v;
    }

    llvm::Value* flt(llvm::Value* a, llvm::Value* b)
    {
        llvm::Value* v = _builder->CreateFCmpOLT(a, b);
        updateFirst(v);
        return v;
    }

private:
    void fsgnj_init(
        llvm::Value* v,
        llvm::Type*& fty,
        llvm::Type*& ity,
        llvm::Constant*& sgn,
        llvm::Constant*& nsgn)
    {
        fty = v->getType();
        xassert(fty == _t->fp32 || fty == _t->fp64);
        ity = fty == _t->fp32? _t->i32 : _t->i64;

        if (ity == _t->i32) {
            sgn = _c->i32(1U<<31);
            nsgn = _c->i32((1U<<31) -1);
        } else {
            sgn = _c->i64(1ULL<<63);
            nsgn = _c->i64((1ULL<<63) -1);
        }
    }

public:
    llvm::Value* fsgnj(llvm::Value* a, llvm::Value* b)
    {
#if 0
        llvm::Type *fty, *ity;
        llvm::Constant *sgn, *nsgn;
        fsgnj_init(a, fty, ity, sgn, nsgn);

        // to int
        a = bitOrPointerCast(a, ity);
        b = bitOrPointerCast(b, ity);
        a = _and(a, nsgn);
        b = _and(b, sgn);
        llvm::Value* v = _or(a, b);
        // to fp
        v = bitOrPointerCast(v, fty);
        return v;
#else
        return callIntrinsic(llvm::Intrinsic::copysign, {a, b});
#endif
    }

    llvm::Value* fsgnjn(llvm::Value* a, llvm::Value* b)
    {
#if 0
        llvm::Type *fty, *ity;
        llvm::Constant *sgn, *nsgn;
        fsgnj_init(a, fty, ity, sgn, nsgn);

        // to int
        a = bitOrPointerCast(a, ity);
        b = bitOrPointerCast(b, ity);
        a = _and(a, nsgn);
        // negate b's signal bit
        b = _xor(b, llvm::ConstantInt::get(ity, ~0ULL));
        b = _and(b, sgn);
        llvm::Value* v = _or(a, b);
        // to fp
        v = bitOrPointerCast(v, fty);
        return v;
#else
        return callIntrinsic(llvm::Intrinsic::copysign, {a, fneg(b)});
#endif
    }

    llvm::Value* fsgnjx(llvm::Value* a, llvm::Value* b)
    {
        llvm::Type *fty, *ity;
        llvm::Constant *sgn, *nsgn;
        fsgnj_init(a, fty, ity, sgn, nsgn);

        // to int
        a = bitOrPointerCast(a, ity);
        b = bitOrPointerCast(b, ity);
        // xor a and b (to get the correct value of the result signal bit)
        b = _xor(a, b);
        b = _and(b, sgn);
        a = _and(a, nsgn);
        llvm::Value* v = _or(a, b);
        // to fp
        v = bitOrPointerCast(v, fty);
        return v;
    }

private:
    llvm::Function* getIntrinsic(
        llvm::Intrinsic::ID id,
        llvm::ArrayRef<llvm::Type*> tys)
    {
        return llvm::Intrinsic::getDeclaration(_ctx->module, id, tys);
    }

    llvm::Value* callIntrinsic(
        llvm::Intrinsic::ID id,
        llvm::ArrayRef<llvm::Value*> args)
    {
        xassert(!args.empty());
        llvm::Value* a = args[0];
        llvm::Function* f = getIntrinsic(id, {a->getType()});
        llvm::Value* v = call(f, args);
        xassert(_first);
        return v;
    }

    // update first instruction pointer
    void updateFirst(llvm::Value* v)
    {
        if (!_first)
            _first = llvm::dyn_cast<llvm::Instruction>(v);
    }
};

}

#endif
