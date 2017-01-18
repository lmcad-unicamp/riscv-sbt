#include "Constants.h"

namespace sbt {

const std::string *BIN_NAME = nullptr;

void initConstants()
{
  BIN_NAME = new std::string("riscv-sbt");
}

void destroyConstants()
{
  delete BIN_NAME;
}

} // sbt
