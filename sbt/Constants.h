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
#define C static const bool
C ADD_NULL = true;
C ALLOW_INTERNAL = true;
C ASSERT_NOT_NULL = true;
C CONSTANT = true;
C DECL = true;
C ERROR = true;
C NO_FIRST = true;
C SIGNED = true;
C VAR_ARG = true;
C VOLATILE = true;
static const char nl = '\n';
#undef C


class Constants
{
public:
    static const uint64_t INVALID_ADDR = ~0ULL;

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

    llvm::ConstantInt* i64(int64_t i) const
    {
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*_ctx), i);
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
