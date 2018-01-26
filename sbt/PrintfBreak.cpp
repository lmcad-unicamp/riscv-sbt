#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {

/**
 * Break complex printf calls into simpler ones.
 *
 * Why is this needed?
 *
 * 1- The SBT has no way to know how many args should be passed to
 *    var arg functions. So it always passes 4. If the correct is less,
 *    they will just be ignored. If it is more, we will have problems.
 *    Thus, we need to break printf's with more than 4 args (excluding the
 *    const char* fmt arg).
 *
 * 2- RISC-V ABI says that 64-bit arguments (such as double) need to be
 *    passed on aligned register pairs, that is, they must start at an even
 *    register number.
 *    For instance, the registers for
 *    1- printf("%f", 1.0) would be: a0, a2 and a3
 *    2- printf("%d %f", 1, 2.0) would be: a0, a1, a2 and a3
 *
 *    The SBT also doesn't know the size of each argument that it should
 *    pass, but this is usually not a problem, because passing a double
 *    word argument and passing 2 single words arguments are usually
 *    equivalent (this is true at least for 32-bit x86 and RISC-V).
 *    However, the alignment requirement is clearly an issue, as there
 *    also no way for the SBT to know if a given argument should be aligned
 *    or not.
 *    For instance, on x86 the 1st printf above would cause the SBT to pass
 *    a0, a1, a2, a3 and a4 (fixed arg + 4) on x86 stack, which will end up
 *    thinking that a1 is the first part of the double arg, which is wrong.
 *
 *    Thus, 64-bit arguments should not be passed directly to printf, to avoid
 *    confusing the translator (although it works on x86 if aligned, we should
 *    not count with this behavior).
 *
 * For now, this is algorithm implemented here:
 *
 * 1- break printf's with more than 4 args but with no double args into
 *    printf's with at most 4 args each (the last one may have less than 4).
 *
 * 2- break printf's with double args at every double argument,
 *    replacing the call to printf with a call to the non-variadic
 *    sbt_printf_d(const char* fmt, double d) function.
 *    If the call also has non-double args, they are broken according to 1,
 *    if needed.
 *    NOTE1: replacing it by printf(fmt, d) would cause 'd' to start in an
 *           unaligned register (a1).
 *    NOTE2: we want the SBT to know that it is handling a 2 words arg to
 *           avoid future issues.
 *
 * While this seems to produce correct and equivalent results, it is probably
 * not the better way to do it for performance.
 *
 * Possible future improvements:
 *
 * 1- Pass two adjacent double args in a single call (sbt_printf_d2)
 * 2- Handle combinations of integers and doubles with less calls:
 *    - int, int, double
 *    - int, double, int
 *    - double, int, int
 */
class PrintfBreak : public FunctionPass
{
public:
    static char ID; // Pass identification, replacement for typeid
    static constexpr const char* PREFIX = "PrintfBreak: ";
    static const char nl = '\n';

    PrintfBreak() :
        FunctionPass(ID)
    {}

    // for each function
    bool runOnFunction(Function& f) override
    {
        _toRemove.clear();
        errs() << PREFIX << "scanning function "
               << f.getName() << '\n';

        // for each basic block
        for (auto& bb : f) {
            // for each instruction
            for (auto& inst : bb) {
                // skip if it's not a call to printf
                auto* call = dyn_cast<CallInst>(&inst);
                if (!call)
                    continue;
                Function* callee = call->getCalledFunction();
                if (callee->getName() != "printf")
                    continue;

                // break it, if needed
                breakPrintfCall(call, callee);
            }
        }

        // remove original calls
        // (we can't do this in the loop because it would invalidate
        //  the iterator)
        for (auto ocall : _toRemove) {
            ocall->dropAllReferences();
            ocall->removeFromParent();
        }

        // return true if we broke at least one call to printf
        return !_toRemove.empty();
    }

private:

    // this struct holds one printf call data
    struct PCall
    {
        std::string fmt;
        unsigned args;
        bool isDouble;

        PCall(std::string&& fmt, unsigned args, bool isDouble) :
            fmt(fmt),
            args(args),
            isDouble(isDouble)
        {}
    };

    // data
    unsigned _i;    // index of argument to be processed next
    unsigned _n;    // total number of arguments
    std::vector<PCall> _pcalls;     // broken calls data
    std::vector<Instruction*> _toRemove;    // instructions to remove

    // prepend our PREFIX in the logs
#   define LOG errs() << PREFIX

    // break a call to printf
    bool breakPrintfCall(CallInst* call, Function* callee)
    {
        unsigned args = call->getNumArgOperands();
        _i = 1;
        _n = args - 1;
        _pcalls.clear();
        if (args == 1) {
            LOG << "1 arg only, no need to break call\n";
            return false;
        }
        LOG << "args=" << _n << nl;

        // get format string
        GlobalVariable* gv;
        Value* v = call->getArgOperand(0);
        auto* ce = dyn_cast<ConstantExpr>(v);
        // TODO possibly support other forms of format string
        assert(ce && ce->getOpcode() == Instruction::MemoryOps::GetElementPtr);

        gv = dyn_cast<GlobalVariable>(ce->getOperand(0));
        // XXX can it be something else?
        assert(gv);

        auto* gvini = dyn_cast<ConstantDataSequential>(gv->getInitializer());
        // XXX can we do anything else for non-initialized global vars?
        assert(gvini && gvini->isCString());

        // log original format string
        StringRef fmt = gvini->getAsCString();
        LOG << "fmt=\"";
        errs().write_escaped(fmt);
        errs() << "\"" << nl;

        // init _pcalls with the whole format string
        PCall pcall = { fmt.str(), args, false };
        for (unsigned j = 1; j < args; j++)
            pcall.args = j;
        _pcalls.emplace_back(std::move(pcall));

        // process args
        unsigned i = 1;
        while (i < args) {
            // handle up to 4 args each time
            unsigned m = std::min(i + 4, args);
            // save initial arg index
            unsigned i0 = i;
            // did a break happen inside the last loop?
            bool broke = false;
            // type of the last arg processed
            Type* ty = nullptr;

            for (; i < m; i++) {
                Value* arg = call->getArgOperand(i);
                // XXX can it ever be null?
                assert(arg);
                ty = arg->getType();
                // XXX float should always be promoted to double in calls to
                //     printf
                assert(!ty->isFloatTy());

                // need to break now?
                bool doBreak = false;
                if (ty->isIntegerTy()) {
                    // TODO break printf's of long longs
                    assert(ty->getIntegerBitWidth() <= 32);
                } else if (ty->isDoubleTy())
                    doBreak = true;

                // break it!
                if (doBreak) {
                    // For now, we end up here only when we find a double
                    // argument. So first "flush" any previous integer args.
                    unsigned args = i - i0;
                    if (args)
                        split(args, false);

                    // now handle the double
                    split(1, ty->isDoubleTy());
                    // don't forget to increment i, because we're also
                    // going to break out of this loop now
                    i++;
                    broke = true;
                    break;
                }
            }

            // if we didn't break anything, it means:
            // 1- 4 args have passed without a break, so do it now
            // 2- we have run out of args, so call split() to handle
            //    this last part (although we're not breaking or splitting
            //    anything in this case...)
            if (!broke)
                split(i - i0, ty->isDoubleTy());
        }

        // now use _pcalls info to issue new printf calls
        issueNewCalls(call);
        return true;
    }


    // split format string and corresponding args at the
    // current index (_i).
    //
    // n - number of args to include in the next pcall
    // isDouble - true if we're splitting on a double arg
    void split(unsigned n, bool isDouble)
    {
        // XXX if current arg is a double, it should be
        //     handled 1 at each time, at least for now
        assert(!isDouble || isDouble && n == 1);

        // save index to pcalls last item
        size_t pi = _pcalls.size() - 1;
        // copy the remaining format string
        const auto s = _pcalls.back().fmt;

        // advance 'pos' until the n+1'th '%' char
        // (or until the end of the string)
        unsigned count = 0;
        unsigned pos = 0;
        for (; pos < s.size(); pos++) {
            char c = s[pos];

            if (c == '%') {
                pos++;
                // XXX the format string should never end with a '%'
                assert(pos < s.size());
                // check next char, to see if this is not an escaped %
                c = s[pos];
                if (c != '%')
                    count++;
                // got enough already?
                if (count == n + 1) {
                    // rewind 'pos' to '%'
                    pos--;
                    break;
                }
            }
        }
        // XXX post conditions:
        //     1- we must have found n + 1 '%' chars
        //     2- or we must have found n '%' chars and reached the end of the
        //        string
        // Otherwise, our algorithm is broken!
        assert(count == n + 1 || count == n && pos == s.size());

        if (count != n) {
            _pcalls.back() = {s.substr(0, pos), n, isDouble};
            _pcalls.emplace_back(s.substr(pos), _n - n, false);
        } else if (isDouble) {
            auto& pcall = _pcalls[pi];
            pcall.isDouble = isDouble;
            pcall.args = n;
        }
        _i += n;

        // dump processed entry
        const auto& pcall = _pcalls[pi];
        LOG << pi << " = { fmt=\"";
        errs().write_escaped(pcall.fmt);
        errs() << "\", args=" << pcall.args << " }\n";
    }


    // issue new printf calls, using _pcalls info
    void issueNewCalls(CallInst* ocall)
    {
        // setup an IRBuilder
        Module* mod = ocall->getModule();
        LLVMContext& ctx = mod->getContext();
        IRBuilder<> builder(ctx);
        // start at original call point
        builder.SetInsertPoint(ocall);

        // setup data that won't change inside the loop
        Type* tyI32 = Type::getInt32Ty(ctx);
        Type* tyDouble = Type::getDoubleTy(ctx);
        Type* tyI8Ptr = Type::getInt8PtrTy(ctx);
        Value* zero = ConstantInt::get(tyI32, 0);
        // XXX ret will hold the sum of all broken
        //     printf's calls.
        // FIXME the code doesn't handle the case where printf
        //       returns a negative value
        Value* ret = builder.CreateAdd(zero, zero);

        auto argit = ocall->arg_begin();
        // skip original format string
        ++argit;

        // for each call
        for (const auto& pcall : _pcalls) {
            // get function to call
            std::vector<Type*> tys = {tyI8Ptr};
            Constant* f;
            // sbt_printf_d for double
            if (pcall.isDouble) {
                tys.push_back(tyDouble);
                FunctionType* ft = FunctionType::get(tyI32, tys, false);
                f = mod->getOrInsertFunction("sbt_printf_d", ft);
            // printf for integers
            } else {
                FunctionType* ft = FunctionType::get(tyI32, {}, true);
                f = mod->getOrInsertFunction("printf", ft);
            }

            // set args
            Value* fmt = builder.CreateGlobalStringPtr(pcall.fmt);
            std::vector<Value*> args = {fmt};
            if (pcall.isDouble) {
                args.push_back(*argit);
                ++argit;
            } else {
                for (unsigned i = 0; i < pcall.args; i++) {
                    args.push_back(*argit);
                    ++argit;
                }
            }

            // call it
            Value* cret = builder.CreateCall(f, args);
            ret = builder.CreateAdd(ret, cret);
        }

        // NOTE make sure there was not arg left
        assert(argit == ocall->arg_end());
        ocall->replaceAllUsesWith(ret);
        // schedule for later removal
        _toRemove.push_back(ocall);
    }

    void getAnalysisUsage(AnalysisUsage& au) const override
    {
        // TODO uncomment this after this code is better tested
        // au.setPreservesCFG();
    }

#   undef LOG
};

}

char PrintfBreak::ID = 0;
static RegisterPass<PrintfBreak> X("printf-break", "Printf Break Pass");
