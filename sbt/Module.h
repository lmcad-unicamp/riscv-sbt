#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Context.h"
#include "Function.h"
#include "Object.h"

#include <llvm/Support/Error.h>

namespace llvm {
class GlobalVariable;
}

namespace sbt {

// this class represents a program module
// (right now, corresponds to one object file)
class Module
{
public:
  Module(Context* ctx);

  // translate object file
  llvm::Error translate(const std::string& file);

private:
  Context* _ctx;
  ConstObjectPtr _obj = nullptr;
  llvm::GlobalVariable* _shadowImage = nullptr;

  // methods

  llvm::Error start();
  llvm::Error finish();
  llvm::Error buildShadowImage();
};

}

#endif
