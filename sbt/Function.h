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
  /**
   * Construct instruction.
   *
   * @param ctx
   * @param name
   * @param sec section of translated binary that contains the function
   * @param addr function address
   * @param end guest memory address of first byte after function's last byte
   *
   * Note: section/end may be null/0 when the function does not correspond
   *       to a guest one.
   */
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

  /**
   * Create the function.
   *
   * @param ft function type
   * @param linkage function linkage
   */
  void create(
    llvm::FunctionType* ft = nullptr,
    llvm::Function::LinkageTypes linkage = llvm::Function::ExternalLinkage);

  // translate function
  llvm::Error translate();


  // getters

  const std::string& name() const
  {
    return _name;
  }

  // llvm function pointer
  llvm::Function* func() const
  {
    return _f;
  }

  uint64_t addr() const
  {
    return _addr;
  }

  // basic block map
  Map<uint64_t, BasicBlock>& bbmap()
  {
    return _bbMap;
  }

  //

  // update next basic block address
  void updateNextBB(uint64_t addr)
  {
    if (_nextBB <= addr || addr < _nextBB)
      _nextBB = addr;
  }

  /**
   * Translate instructions at given address range.
   *
   * @param st starting address
   * @param end end address (after last instruction)
   */
  llvm::Error translateInstrs(uint64_t st, uint64_t end);

  // look up function by address
  static Function* getByAddr(Context* ctx, uint64_t addr);

private:
  Context* _ctx;
  std::string _name;
  SBTSection* _sec;
  uint64_t _addr;
  uint64_t _end;

  llvm::Function* _f = nullptr;
  // current basic block pointer
  BasicBlock* _bb = nullptr;
  // address of next basic block
  uint64_t _nextBB = 0;
  Map<uint64_t, BasicBlock> _bbMap;

  enum TranslationState {
    ST_DFL,     // default
    ST_PADDING  // processing padding bytes
  };

  TranslationState _state = ST_DFL;

  // methods

  llvm::Error startMain();
  llvm::Error start();
  llvm::Error finish();
};

using FunctionPtr = Pointer<Function>;

}

#endif
