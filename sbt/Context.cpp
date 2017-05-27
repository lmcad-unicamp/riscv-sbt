#include "Context.h"

#include "Disassembler.h"
#include "Stack.h"
#include "XRegisters.h"

namespace sbt {

Context::~Context()
{
  // delete owned objects
  delete x.get();
  delete stack;
  delete disasm;
}

}
