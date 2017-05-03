#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Function.h"
#include "Map.h"
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


  llvm::Error start();
  llvm::Error finish();

  TranslationState _state = ST_DFL;
  ConstObjectPtr _obj = nullptr;
  Map<llvm::Function *, uint64_t> _funMap;
  std::vector<llvm::Function *> _funTable;
  // icaller
  llvm::Function *ICaller = nullptr;
  uint64_t ExtFunAddr = 0;
};

}

#endif
