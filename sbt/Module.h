#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Object.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>

namespace llvm {
class GlobalVariable;
class Module;
}

namespace sbt {

class Module
{
public:
  Module(
    llvm::LLVMContext* ctx,
    llvm::IRBuilder<>* builder,
    llvm::Module* module)
    :
    _ctx(ctx),
    _builder(builder),
    _module(module)
  {}

  llvm::Error translate(const std::string& file);

private:
  llvm::LLVMContext* _ctx;
  llvm::IRBuilder<>* _builder;
  llvm::Module* _module;

  ConstObjectPtr _obj = nullptr;

  llvm::GlobalVariable* _shadowImage = nullptr;

  // methods

  llvm::Error start();
  llvm::Error finish();

  // indirect function caller
  llvm::Error genICaller();

  llvm::Error buildShadowImage();

  /*
  Map<llvm::Function *, uint64_t> _funMap;
  std::vector<llvm::Function *> _funTable;
  // icaller
  llvm::Function *ICaller = nullptr;
  uint64_t ExtFunAddr = 0;
  ConstSectionPtr _bss = nullptr;
  size_t _bssSize = 0;
  */
};

}

#endif
