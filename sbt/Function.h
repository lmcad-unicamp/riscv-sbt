#ifndef SBT_FUNCTION_H
#define SBT_FUNCTION_H

#include "BasicBlock.h"
#include "Context.h"
#include "Map.h"
#include "Object.h"
#include "Pointer.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <memory>


namespace llvm {
class BasicBlock;
class Function;
class Instruction;
class Module;
}

namespace sbt {

class SBTSection;

class Function
{
public:
  Function(
    Context* ctx,
    const std::string& name,
    SBTSection* sec = nullptr,
    uint64_t addr = 0,
    uint64_t end = 0)
    :
    _ctx(ctx),
    _name(name),
    _sec(sec),
    _addr(addr),
    _end(end)
  {}

  llvm::Error create(
    llvm::FunctionType* ft = nullptr,
    llvm::Function::LinkageTypes linkage = llvm::Function::ExternalLinkage);

  llvm::Error translate();


  const std::string& name() const
  {
    return _name;
  }

  llvm::Function* func() const
  {
    return _f;
  }

  uint64_t addr() const
  {
    return _addr;
  }

private:
  Context* _ctx;
  std::string _name;
  SBTSection* _sec;
  uint64_t _addr;
  uint64_t _end;

  llvm::Function* _f = nullptr;
  BasicBlock* _bb = nullptr;
  Map<uint64_t, BasicBlock> _bbMap;

  enum TranslationState {
    ST_DFL,
    ST_PADDING
  };

  TranslationState _state = ST_DFL;


  llvm::Error startMain();
  llvm::Error start();
  llvm::Error finish();

  llvm::Error translateInstrs(uint64_t st, uint64_t end);

  /*
  llvm::Expected<uint64_t> import(llvm::StringRef func);


private:
  uint64_t _nextBB = 0;
  bool _brWasLast = false;
  */
};

using FunctionPtr = Pointer<Function>;

}

#endif
