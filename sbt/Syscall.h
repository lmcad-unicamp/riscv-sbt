#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>

namespace llvm {
class Function;
class FunctionType;
class Module;
}

namespace sbt {

class Syscall
{
public:
  void declHandler(llvm::Module* module);

  llvm::Error genHandler(
    llvm::LLVMContext* ctx,
    llvm::IRBuilder<>* builder,
    llvm::Module* module);

private:
  // riscv syscall function
  llvm::FunctionType* _ftRVSC;
  llvm::Function* _fRVSC;
};

}
