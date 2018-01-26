#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {

class PrintfBreak : public FunctionPass
{
public:
    static char ID; // Pass identification, replacement for typeid
    static constexpr const char* PREFIX = "PrintfBreak: ";
    static const char nl = '\n';

    PrintfBreak() :
        FunctionPass(ID)
    {}

    bool runOnFunction(Function& f) override
    {
        errs() << PREFIX << "scanning function "
               << f.getName() << '\n';

        for (auto& bb : f) {
            for (auto& inst : bb) {
                auto* call = dyn_cast<CallInst>(&inst);
                if (!call)
                    continue;

                Function* callee = call->getCalledFunction();
                if (callee->getName() != "printf")
                    continue;
                breakPrintfCall(call, callee);
            }
        }

        return false;
    }

private:
#   define LOG errs() << PREFIX

    void breakPrintfCall(CallInst* call, Function* callee)
    {
        // LOG << "breaking call to " << callee->getName() << nl;

        unsigned n = call->getNumArgOperands();
        if (n == 1) {
            LOG << "1 arg only, no need to break call" << nl;
            return;
        }
        LOG << "args=" << n << nl;

        // get format string
        GlobalVariable* gv;
        Value* v = call->getArgOperand(0);
        auto* ce = dyn_cast<ConstantExpr>(v);
        assert(ce && ce->getOpcode() == Instruction::MemoryOps::GetElementPtr);

        gv = dyn_cast<GlobalVariable>(ce->getOperand(0));
        assert(gv);

        auto* gvini = dyn_cast<ConstantDataSequential>(gv->getInitializer());
        assert(gvini && gvini->isCString());

        StringRef fmt = gvini->getAsCString();
        LOG << "fmt=\"";
        errs().write_escaped(fmt);
        errs() << "\"" << nl;

        // process args
        unsigned i = 1;
        std::vector<std::string> fmts;
        fmts.push_back(fmt.str());

        auto do_break = [this, &fmts](unsigned n) {
            // LOG << "new " << n << " args block" << nl;
            splitString(fmts, n);
        };

        while (i < n) {
            unsigned m = std::min(i + 4, n);
            unsigned i0 = i;
            bool broke = false;
            for (; i < m; i++) {
                Value* arg = call->getArgOperand(i);
                assert(arg);
                Type* ty = arg->getType();
                assert(!ty->isFloatTy());

                bool doBreak = false;
                if (ty->isIntegerTy()) {
                    if (ty->getIntegerBitWidth() > 32)
                        doBreak = true;
                } else if (ty->isDoubleTy())
                    doBreak = true;

                if (doBreak) {
                    unsigned args = i - i0;
                    if (args)
                        do_break(args);

                    do_break(1);
                    i++;
                    broke = true;
                    break;
                }
            }

            if (!broke)
                do_break(i - i0);
        }
    }


    void splitString(std::vector<std::string>& v, unsigned n)
    {
        size_t vi = v.size() - 1;
        const auto s = v.back();

        unsigned count = 0;
        unsigned pos = 0;
        for (; pos < s.size(); pos++) {
            char c = s[pos];

            if (c == '%') {
                pos++;
                assert(pos < s.size());
                c = s[pos];
                if (c != '%')
                    count++;
                if (count == n + 1) {
                    pos--;
                    break;
                }
            }
        }
        assert(count == n + 1 || count == n && pos == s.size());

        if (count != n) {
            v.back() = s.substr(0, pos);
            v.push_back(s.substr(pos));
        }

        LOG << "fmt_block=\"";
        errs().write_escaped(v[vi]);
        errs() << "\"" << nl;
    }

#   undef LOG
};

}

char PrintfBreak::ID = 0;
static RegisterPass<PrintfBreak> X("printf-break", "Printf Break Pass");
