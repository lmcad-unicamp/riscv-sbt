#include "Context.h"

#include "Disassembler.h"
#include "FRegister.h"
#include "Function.h"
#include "Stack.h"
#include "XRegister.h"

namespace sbt {

Context::~Context()
{
    // delete owned objects
    delete f;
    delete x;
    delete stack;
    delete disasm;
}

void Context::addFunc(FunctionPtr&& f)
{
    _funcByAddr->upsert(f->addr(), std::move(&*f));
    std::string fname = f->name();
    _func->upsert(std::move(fname), std::move(f));
}

}
