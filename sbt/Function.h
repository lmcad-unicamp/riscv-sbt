#ifndef SBT_FUNCTION_H
#define SBT_FUNCTION_H

#include "Map.h"
#include "Object.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>

#include <memory>


namespace llvm {
class BasicBlock;
class Function;
class Instruction;
class Module;
}

namespace sbt {

class Function
{
public:
  Function(
    const llvm::StringRef name,
    uint64_t addr,
    uint64_t end,
    llvm::Module* mod,
    llvm::IRBuilder<>* builder,
    llvm::LLVMContext* ctx)
    :
    _name(name),
    _addr(addr),
    _end(end),
    _module(mod),
    _builder(builder),
    _ctx(ctx)
  {}

  llvm::Error translate();

private:
  const llvm::StringRef _name;
  uint64_t _addr;
  uint64_t _end;
  llvm::Module* _module;
  llvm::IRBuilder<>* _builder;
  llvm::LLVMContext* _ctx;


  llvm::Error startMain();
  llvm::Error start() { return llvm::Error::success(); }
  llvm::Error finish() { return llvm::Error::success(); }

  llvm::Error translateInstrs(uint64_t st, uint64_t end)
  { return llvm::Error::success(); }

  /*
  llvm::Expected<llvm::Function *> create(llvm::StringRef name);
  llvm::Expected<uint64_t> import(llvm::StringRef func);


private:
  uint64_t _addr = 0;
  Map<uint64_t, llvm::BasicBlock *> _bbMap;
  uint64_t _nextBB = 0;
  bool _brWasLast = false;
  */
};

using FunctionPtr = std::unique_ptr<Function>;

}

#endif
