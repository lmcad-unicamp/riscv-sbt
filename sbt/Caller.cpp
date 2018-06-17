#include "Caller.h"

#include "Builder.h"
#include "Debug.h"
#include "FRegister.h"
#include "Function.h"
#include "Options.h"
#include "XRegister.h"

#include <llvm/IR/ValueSymbolTable.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

size_t Caller::MAX_ARGS;


llvm::Value* Caller::getFunctionSymbol(
    Context* ctx,
    const std::string& name)
{
    return ctx->module->getValueSymbolTable().lookup(name);
}


Caller::Caller(
    Context* ctx,
    Builder* bld,
    Function* tgtF,
    Function* curF)
    :
    _ctx(ctx),
    _bld(bld),
    _tgtF(tgtF),
    _curF(curF),
    _hf(_ctx->opts->hardFloatABI()),
    _reg(XRegister::A0),
    _freg(FRegister::FA0)
{
    xassert(_ctx);
    xassert(_bld);
    xassert(_tgtF);
    xassert(_curF);

    // get needed function info
    llvm::Function* llf = _tgtF->func();
    _llft = llf->getFunctionType();
    // lookup function by symbol
    llvm::Value* sym = getFunctionSymbol(_ctx, _tgtF->name());
    xassert(sym);
    // get pointer to function
    llvm::PointerType* fty = _llft->getPointerTo();
    _fptr = _bld->bitOrPointerCast(sym, fty);

    // prepare args
    _fixedArgs = _llft->getNumParams();
    _wordArgs = 0;
    _isVarArg = _llft->isVarArg();
    for (const auto& ty : _llft->params())
        if ((!_hf || _isVarArg) && ty->isDoubleTy())
            _wordArgs += 2;
        else
            _wordArgs += 1;

    // varArgs: passing 4 extra args for now
    if (_isVarArg) {
        _totalArgs = MIN(_fixedArgs + 4, MAX_ARGS);
        _wordArgs = MIN(_wordArgs + 4, MAX_ARGS);
    } else
        _totalArgs = _fixedArgs;
}


llvm::Value* Caller::nextArg()
{
    // get zero or register
    if (_passZero)
        return _ctx->c.ZERO;

    llvm::Value* v;
    if (_args) {
        if (_argit == _args->end()) {
            _passZero = true;
            return _ctx->c.ZERO;
        }
        v = *_argit;
        ++_argit;
        return v;
    }

    llvm::Type* ty;
    if (_hf && _argIdx < _fixedArgs)
        ty = _llft->getParamType(_argIdx++);
    else
        ty = _ctx->t.i32;

    if (ty->isFloatTy())
        v = _bld->fload32(_freg);
    else if (ty->isDoubleTy())
        v = _bld->fload64(_freg);
    else
        v = _bld->load(_reg);

    if (ty->isFloatTy() || ty->isDoubleTy())
        _freg++;
    else
        _reg++;

    return v;
}


llvm::Value* Caller::castArg(llvm::Value* v, llvm::Type* ty)
{
    // check if conversion can be skipped
    if (_hf) {
        if (_isVarArg) {
            if (ty == _ctx->t.i32)
                return v;
        } else {
            if (ty == _ctx->t.i32 || ty->isFloatTy() || ty->isDoubleTy())
                return v;
        }
    } else {
        if (ty == _ctx->t.i32)
            return v;
    }

    DBGF("cast from:");
    DBG(v->getType()->dump());
    DBGF("cast to:");
    DBG(ty->dump());
    DBGS.flush();

    if (ty->isDoubleTy())
        return i32x2ToFP64(v, nextArg());
    // long double: by ref
    if (ty->isFP128Ty())
        return refToFP128(v);
    return _bld->bitOrPointerCast(v, ty);
}


void Caller::setArgs(std::vector<llvm::Value*>* args)
{
    _args = args;
    _argit = _args->begin();
}


void Caller::callExternal()
{
    // get return type
    llvm::Type* retty = _llft->getReturnType();

    // types > 2xi32 are returned by ref and the address is passed by the
    // caller in the first argument
    if (retty->isFP128Ty())
        _retref = nextArg();
    std::vector<llvm::Value*> args;
    args.reserve(_totalArgs);

    // pass args
    for (size_t i = 0; i < _totalArgs; i++) {
        llvm::Value* v = nextArg();
        llvm::Type* ty = i < _fixedArgs? _llft->getParamType(i) : _ctx->t.i32;
        args.push_back(castArg(v, ty));
    }

    // dump args
    // for (auto arg : args)
    //    arg->dump();

    llvm::Value* ret = _bld->call(_fptr, args);
    handleReturn(ret);
}


Register& Caller::getRetReg(unsigned reg)
{
    if (_retInGlobal)
        return _ctx->x->getReg(reg);
    else
        return _curF->getReg(reg);
}


Register& Caller::getFRetReg(unsigned reg)
{
    if (_retInGlobal)
        return _ctx->f->getReg(reg);
    else
        return _curF->getFReg(reg);
}


void Caller::handleReturn(llvm::Value* ret)
{
    llvm::Type* retty = ret->getType();

    if (retty->isVoidTy())
        return;

    if (_hf && retty->isFloatTy()) {
        auto& reg = getFRetReg(FRegister::FA0);
        _bld->store(ret, _bld->fp64PtrToFP32Ptr(reg.getForWrite()));
    } else if (retty->isDoubleTy()) {
        if (_hf) {
            auto& reg = getFRetReg(FRegister::FA0);
            _bld->store(ret, reg.getForWrite());
        } else {
            auto p = fp64ToI32x2(ret);
            auto& reglo = getRetReg(XRegister::A0);
            auto& reghi = getRetReg(XRegister::A1);
            _bld->store(p.first, reglo.getForWrite());
            _bld->store(p.second, reghi.getForWrite());
        }
    } else if (retty->isFP128Ty()) {
        xassert(_retref);
        fp128ToRef(ret, _retref);
    } else {
        if (retty != _ctx->t.i32)
            ret = _bld->bitOrPointerCast(ret, _ctx->t.i32);
        auto& reg = getRetReg(XRegister::A0);
        _bld->store(ret, reg.getForWrite());
    }
}


llvm::Value*
Caller::i32x2ToFP64(llvm::Value* lo, llvm::Value* hi)
{
    llvm::Value* vlo = _bld->zext64(lo);
    llvm::Value* vhi = _bld->zext64(hi);
    vhi = _bld->sll(vhi, _ctx->c.i64(32));
    // merge hi and low
    llvm::Value* v = _bld->_or(vhi, vlo);
    // then finally cast to double
    v = _bld->bitOrPointerCast(v, _ctx->t.fp64);
    return v;
}


std::pair<llvm::Value*, llvm::Value*>
Caller::fp64ToI32x2(llvm::Value* f)
{
    llvm::Value* v = _bld->bitOrPointerCast(f, _ctx->t.i64);
    llvm::Value* vlo = _bld->truncOrBitCastI32(v);
    llvm::Value* vhi = _bld->srl(v, _ctx->c.i64(32));
    vhi = _bld->truncOrBitCastI32(vhi);
    return std::make_pair(vlo, vhi);
}


llvm::Value* Caller::refToFP128(llvm::Value* ref)
{
    llvm::Value* ptr = _bld->bitOrPointerCast(ref, _ctx->t.fp128ptr);
    return _bld->load(ptr);
}


void Caller::fp128ToRef(llvm::Value* f, llvm::Value* ref)
{
    llvm::Value* ptr = _bld->bitOrPointerCast(ref, _ctx->t.fp128ptr);
    _bld->store(f, ptr);
}

}
