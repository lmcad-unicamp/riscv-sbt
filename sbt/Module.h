#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Object.h"

#include <llvm/Support/Error.h>

namespace sbt {

class Module
{
public:

  // State

  enum TranslationState {
    ST_DFL,
    ST_PADDING
  };

  llvm::Error translate(ConstObjectPtr obj);

private:
  ConstObjectPtr _obj = nullptr;
  TranslationState _state = ST_DFL;

  // methods

  llvm::Error start();
  llvm::Error finish();

  // indirect function caller
  llvm::Error genICaller();

  llvm::Error buildShadowImage();

  /*
  bool inFunc() const
  {
    return _func;
  }

  Map<llvm::Function *, uint64_t> _funMap;
  std::vector<llvm::Function *> _funTable;
  // icaller
  llvm::Function *ICaller = nullptr;
  uint64_t ExtFunAddr = 0;
  llvm::GlobalVariable* _shadowImage = nullptr;
  ConstSectionPtr _bss = nullptr;
  size_t _bssSize = 0;
  */
};

}

#endif
