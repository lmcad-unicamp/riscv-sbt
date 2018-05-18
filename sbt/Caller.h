#ifndef SBT_CALLER_H
#define SBT_CALLER_H

#include "Context.h"

namespace sbt {

class Register;

class Caller
{
public:
    Caller(
        Context* ctx,
        Builder* bld,
        Function* tgtF,
        Function* curF);

    void callExternal();
    void setArgs(std::vector<llvm::Value*>* args);

    size_t getTotalArgs() const
    {
        return _totalArgs;
    }

    void setRetInGlobal(bool b)
    {
        _retInGlobal = b;
    }

    static size_t MAX_ARGS;

private:
    Context* _ctx;
    Builder* _bld;
    Function* _tgtF;
    Function* _curF;
    std::vector<llvm::Value*>* _args = nullptr;

    llvm::FunctionType* _llft;
    llvm::Value* _fptr;
    unsigned _reg;
    bool _passZero = false;
    std::vector<llvm::Value*>::const_iterator _argit;
    size_t _fixedArgs;
    size_t _totalArgs;
    llvm::Value* _retref = nullptr;
    bool _retInGlobal = false;


    llvm::Value* nextArg();
    llvm::Value* castArg(llvm::Value* v, llvm::Type* ty);
    void handleReturn(llvm::Value* ret);
    Register& getRetReg(unsigned reg);

    llvm::Value* i32x2ToFP64(llvm::Value* lo, llvm::Value* hi);
    std::pair<llvm::Value*, llvm::Value*> fp64ToI32x2(llvm::Value* f);
    llvm::Value* refToFP128(llvm::Value* ref);
    void fp128ToRef(llvm::Value* f, llvm::Value* ref);
};

}

#endif
