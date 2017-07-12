#include "Syscall.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Constants.h"

#include <llvm/IR/Function.h>

#include <vector>

namespace sbt {

void Syscall::declHandler()
{
    std::vector<llvm::Type*> params;
    params.reserve(MAX_ARGS);
    for (size_t i = 0; i < MAX_ARGS; i++)
        params.push_back(_t.i32);

    _ftRVSC = llvm::FunctionType::get(_t.i32, params, !VAR_ARG);
    _fRVSC = llvm::Function::Create(
        _ftRVSC, llvm::Function::ExternalLinkage, "rv_syscall",
        _ctx->module);
}


void Syscall::genHandler()
{
    llvm::Module* module = _ctx->module;
    const Constants& c = _ctx->c;

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
        { 1, 93, 1 },    // EXIT
        { 3, 64, 4 }     // WRITE
    };

    const int X86_SYS_EXIT = 1;
    const std::string bbPrefix = "bb_rvsc_";

    declHandler();

    // prepare
    Builder* bld = _ctx->bld;
    xassert(bld);
    bld->saveInsertBlock();
    bld->reset();
    std::vector<llvm::Value*> args;

    // entry
    BasicBlock bbEntry(_ctx, bbPrefix + "entry", _fRVSC);
    // 1st arg: syscall #
    auto argit = _fRVSC->arg_begin();
    llvm::Argument &sc = *argit;
    ++argit;

    // default case: call exit(99)
    BasicBlock bbDfl(_ctx, bbPrefix + "default", _fRVSC);
    bld->setInsertPoint(bbDfl);
    args.clear();
    args.reserve(2);
    args.push_back(c.i32(X86_SYS_EXIT));
    args.push_back(c.i32(99));
    bld->call(fX86SC[1], args);
    bld->ret(c.ZERO);

    // switch (RISC-V syscall#)

    bld->setInsertPoint(bbEntry);
    llvm::SwitchInst *sw1 = bld->sw(&sc, bbDfl);

    auto setArgs = [this, &args](llvm::Value* sc, size_t n) {
        args.clear();
        args.reserve(n+1);
        args.push_back(sc);

        auto argit = _fRVSC->arg_begin();
        ++argit;    // skip guest sc#
        for (size_t i = 0; i < n; i++, ++argit)
            args.push_back(&*argit);
    };

    // sw cases
    auto addCase = [&](const Syscall& s) {
        std::string sss = bbPrefix;
        llvm::raw_string_ostream ss(sss);
        ss << "case_" << s.rv;

        BasicBlock bb(_ctx, ss.str(), _fRVSC, bbDfl.bb());
        bld->setInsertPoint(bb);
        DBGF("processing syscall: args={0}, rv={1}, x86={2}",
            s.args, s.rv, s.x86);
        setArgs(c.i32(s.x86), s.args);
        llvm::Value* v = bld->call(fX86SC[s.args], args);
        bld->ret(v);
        sw1->addCase(_ctx->c.i32(s.rv), bb.bb());
    };

    for (const Syscall& s : scv)
        addCase(s);

    bld->restoreInsertBlock();

    // _fRVSC->dump();
}


void Syscall::call()
{
    Builder* bld = _ctx->bld;
    Function* f = _ctx->f;
    xassert(f);

    DBGF("call");

    // set args
    llvm::Value* sc = bld->load(XRegister::A7);
    std::vector<llvm::Value*> args = { sc };
    size_t i = 1;
    size_t reg = XRegister::A0;
    for (; i < MAX_ARGS; i++, reg++) {
        // XXX
        if (!f->getReg(reg).hasWrite())
            break;
        args.push_back(bld->load(reg));
    }
    for (; i < MAX_ARGS; i++)
        args.push_back(_ctx->c.ZERO);

    llvm::Value* v = bld->call(_fRVSC, args);
    bld->store(v, XRegister::A0);
}

}
