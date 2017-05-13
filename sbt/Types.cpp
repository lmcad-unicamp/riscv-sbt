#include "Types.h"

#include "Constants.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>

namespace sbt {

Types::Types(llvm::LLVMContext& ctx) :
  voidT(llvm::Type::getVoidTy(ctx)),
  i8(llvm::Type::getInt8Ty(ctx)),
  i16(llvm::Type::getInt16Ty(ctx)),
  i32(llvm::Type::getInt32Ty(ctx)),
  i8ptr(llvm::Type::getInt8PtrTy(ctx)),
  i16ptr(llvm::Type::getInt16PtrTy(ctx)),
  i32ptr(llvm::Type::getInt32PtrTy(ctx)),
  voidFunc(llvm::FunctionType::get(voidT, !VAR_ARG))
{}

}
