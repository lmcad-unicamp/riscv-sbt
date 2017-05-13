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

  llvm::PointerType* i8ptr;
  llvm::PointerType* i16ptr;
  llvm::PointerType* i32ptr;

  llvm::FunctionType* voidFunc;


  Types(llvm::LLVMContext& ctx);
};

}

#endif
