#ifndef SBT_CONSTANTS_H
#define SBT_CONSTANTS_H

#include "Debug.h"

#include <llvm/IR/Constants.h>

#include <string>

namespace llvm {
class LLVMContext;
}

namespace sbt {

// (these are only to make the code easier to read)
static const bool ADD_NULL = true;
static const bool ALLOW_INTERNAL = true;
static const bool CONSTANT = true;
static const bool DECL = true;
static const bool ERROR = true;
static const bool NO_FIRST = true;
static const bool SIGNED = true;
static const bool VAR_ARG = true;
static const bool VOLATILE = true;
static const char nl = '\n';


class Constants
{
public:
  // name of the SBT binary/executable
  const std::string BIN_NAME = "riscv-sbt";
  // int32 zero
  llvm::ConstantInt* ZERO = nullptr;

  /**
   * Init Constants instance.
   * Except for BIN_NAME, all other members may be
   * uninitialized before this call.
   */
  void init(llvm::LLVMContext* ctx);

  // get path to libc.bc
  const std::string& libCBC() const
  {
    return _libCBC;
  }

  // get a constant int32 llvm value
  llvm::ConstantInt* i32(int32_t i) const
  {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*_ctx), i);
  }

  /**
   * Get global Constants instance.
   *
   * This method should not be used to get anything that depends
   * on the LLVMContext used to initialize an instance of Constants.
   */
  static const Constants& global()
  {
    static Constants c;
    return c;
  }

private:
  std::string _libCBC;
  llvm::LLVMContext* _ctx = nullptr;
};

} // sbt

#endif
