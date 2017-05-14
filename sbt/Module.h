#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Context.h"
#include "Function.h"
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
  Module(Context* ctx);

  llvm::Error translate(const std::string& file);

private:
  Context* _ctx;
  ConstObjectPtr _obj = nullptr;
  llvm::GlobalVariable* _shadowImage = nullptr;
  // icaller
  Function _iCaller;
  Map<FunctionPtr, uint64_t> _funMap;

  // methods

  llvm::Error start();
  llvm::Error finish();

  // indirect function caller
  // FIXME this probably should be moved to Translator, in order
  //       to handle indirect calls between different modules
  llvm::Error genICaller();

  llvm::Error buildShadowImage();
};

}

#endif
