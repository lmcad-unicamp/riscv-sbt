#include "Context.h"
#include "XRegisters.h"

namespace sbt {

Context::~Context()
{
  delete x.get();
}

}
