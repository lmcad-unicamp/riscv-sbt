#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Context.h"
#include "Function.h"
#include "Object.h"

#include <llvm/Support/Error.h>

#include <memory>


namespace sbt {

// this class represents a program module
// (right now, corresponds to one object file)
class Module
{
public:
  Module(Context* ctx);
  ~Module();

  // translate object file
  llvm::Error translate(const std::string& file);

private:
  Context* _ctx;
  ConstObjectPtr _obj = nullptr;
  std::unique_ptr<ShadowImage> _shadowImage;

  // methods

  void start();
  void finish() {}
};

}

#endif
