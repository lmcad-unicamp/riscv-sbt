#ifndef SBT_TYPES_H
#define SBT_TYPES_H

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

class Types
{
public:
  llvm::Type* voidT;

  llvm::IntegerType* i8;
  llvm::IntegerType* i16;
  llvm::IntegerType* i32;
  llvm::IntegerType* i64;

  llvm::PointerType* i8ptr;
  llvm::PointerType* i16ptr;
  llvm::PointerType* i32ptr;

  llvm::FunctionType* voidFunc;

  llvm::Type* fp32;
  llvm::Type* fp64;
  llvm::Type* fp128;
  llvm::Type* fp32ptr;
  llvm::Type* fp64ptr;
  llvm::Type* fp128ptr;

  Types(llvm::LLVMContext& ctx);
};

}

#endif
