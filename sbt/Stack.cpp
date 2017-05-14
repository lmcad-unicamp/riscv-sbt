#include "Stack.h"

#include "Builder.h"
#include "Context.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>

#include <string>

namespace sbt {

Stack::Stack(Context* ctx, size_t sz)
  :
  _size(sz)
{
  std::string bytes(_size, 'S');

  llvm::ArrayRef<uint8_t> byteArray(
    reinterpret_cast<const uint8_t *>(bytes.data()),
    _size);

  llvm::Constant *cda = llvm::ConstantDataArray::get(*ctx->ctx, byteArray);

  _stack = new llvm::GlobalVariable(
    *ctx->module, cda->getType(), !CONSTANT,
      llvm::GlobalValue::ExternalLinkage, cda, "Stack");

  // set stack end pointer

  const Constants& c = ctx->c;
  Builder bld(ctx);
  std::vector<llvm::Value*> idx = { c.ZERO, c.i32(_size) };
  llvm::Value *v = ctx->builder->CreateGEP(_stack, idx);
  _end = bld.i8PtrToI32(v);
}

}
