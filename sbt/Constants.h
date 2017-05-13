#ifndef SBT_CONSTANTS_H
#define SBT_CONSTANTS_H

#define SBT_DEBUG 1

#include <string>

namespace llvm {
class ConstantInt;
class LLVMContext;
}

namespace sbt {

// name of the SBT binary/executable

// (these are only to make the code easier to read)
static const bool ADD_NULL = true;
static const bool ALLOW_INTERNAL = true;
static const bool CONSTANT = true;
static const bool DECL = true;
static const bool ERROR = true;
static const bool SIGNED = true;
static const bool VAR_ARG = true;
static const bool VOLATILE = true;
static const char nl = '\n';


class Constants
{
public:
  const std::string BIN_NAME = "riscv-sbt";
  llvm::ConstantInt* ZERO = nullptr;

  const std::string& libCBC() const
  {
    return _libCBC;
  }

  void init(llvm::LLVMContext& ctx);

  static const Constants& global()
  {
    static Constants c;
    return c;
  }

private:
  std::string _libCBC;
};

} // sbt

#endif
