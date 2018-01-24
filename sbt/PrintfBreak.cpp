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

    PrintfBreak() :
        FunctionPass(ID)
    {}

    bool runOnFunction(Function& f) override
    {
        errs() << __FUNCTION__ << "(): "
               << f.getName() << '\n';

        for (auto& bb : f) {
            for (auto& inst : bb) {
                auto* call = dyn_cast<CallInst>(&inst);
                if (!call)
                    continue;

                Function* callee = call->getCalledFunction();
                errs() << __FUNCTION__ << "(): call to "
                       << callee->getName() << '\n';
            }
        }

        return false;
    }

private:

};

}

char PrintfBreak::ID = 0;
static RegisterPass<PrintfBreak> X("printf-break", "Printf Break Pass");
