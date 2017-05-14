#include "Context.h"

#include "Stack.h"
#include "XRegisters.h"

namespace sbt {

Context::~Context()
{
  delete x.get();
  delete stack;
}

}
