#ifndef SBT_FUNCTION_H
#define SBT_FUNCTION_H

#include "Map.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <memory>


namespace llvm {
class BasicBlock;
class Function;
class Instruction;
}

namespace sbt {

class Function
{
public:

  /*
  llvm::Expected<llvm::Function *> create(llvm::StringRef name);
  llvm::Error startMain(llvm::StringRef name, uint64_t addr);
  llvm::Error start(llvm::StringRef name, uint64_t addr);
  llvm::Error finish();
  llvm::Expected<uint64_t> import(llvm::StringRef func);


private:
  uint64_t _addr = 0;
  Map<uint64_t, llvm::BasicBlock *> _bbMap;
  Map<uint64_t, llvm::Instruction *> _instrMap;
  uint64_t _nextBB = 0;
  bool _brWasLast = false;
  */
};

using FunctionPtr = std::unique_ptr<Function>;

}

#endif
