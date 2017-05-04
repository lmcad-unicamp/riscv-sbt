#ifndef SBT_CONSTANTS_H
#define SBT_CONSTANTS_H

#define SBT_DEBUG 1

#include <string>

namespace llvm {
class ConstantInt;
class FunctionType;
class IntegerType;
class LLVMContext;
class PointerType;
class Type;
class Value;
}

namespace sbt {

// name of the SBT binary/executable
extern const std::string* BIN_NAME;
extern const std::string* LIBC_BC;

// (these are only to make the code easier to read)
static const bool ADD_NULL = true;
static const bool ALLOW_INTERNAL = true;
static const bool CONSTANT = true;
static const bool ERROR = true;
static const bool SIGNED = true;
static const bool VAR_ARG = true;
static const bool VOLATILE = true;
static const char nl = '\n';

void initConstants();
void destroyConstants();


// LLVM constants

extern llvm::Type* Void;

extern llvm::IntegerType* I8;
extern llvm::IntegerType* I16;
extern llvm::IntegerType* I32;

extern llvm::PointerType* I8Ptr;
extern llvm::PointerType* I16Ptr;
extern llvm::PointerType* I32Ptr;

extern llvm::FunctionType* VoidFun;

extern llvm::ConstantInt* ZERO;

void initLLVMConstants(llvm::LLVMContext& ctx);

} // sbt

#endif
