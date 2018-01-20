#include "Context.h"

#include "Disassembler.h"
#include "Function.h"
#include "Stack.h"
#include "XRegister.h"

namespace sbt {

Context::~Context()
{
  // delete owned objects
  delete x;
  delete stack;
  delete disasm;
}

void Context::addFunc(FunctionPtr&& f)
{
  (*_funcByAddr)(f->addr(), std::move(&*f));
  std::string fname = f->name();
  (*_func)(std::move(fname), std::move(f));
}

}
