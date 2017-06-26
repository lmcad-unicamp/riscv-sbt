#ifndef SBT_BASICBLOCK_H
#define SBT_BASICBLOCK_H

#include "Context.h"
#include "Map.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

namespace sbt {

class BasicBlock;
using BasicBlockPtr = Pointer<BasicBlock>;

class BasicBlock
{
public:
  /**
   * @param ctx
   * @param addr basic block address (used to build its name only)
   * @param f function that own the basic block
   * @param beforeBB
   */
  BasicBlock(
    Context* ctx,
    uint64_t addr,
    Function* f,
    BasicBlock* beforeBB = nullptr);

  /**
   * Same as above, but with llvm types.
   */
  BasicBlock(
    Context* ctx,
    uint64_t addr,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr)
    :
    BasicBlock(ctx, getBBName(addr), f, beforeBB)
  {
    _addr = addr;
  }

  /**
   * Construct basic block with a name not related to an address.
   */
  BasicBlock(
    Context* ctx,
    llvm::StringRef name,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr);

  /**
   * Construct from existing llvm::BasicBlock
   */
  BasicBlock(
    Context* ctx,
    llvm::BasicBlock* bb,
    uint64_t addr)
    :
    _ctx(ctx),
    _bb(bb),
    _addr(addr)
  {}

public:
  ~BasicBlock();

  // allow move only
  BasicBlock(BasicBlock&&) = default;
  BasicBlock& operator=(BasicBlock&&) = default;

  // getters

  llvm::BasicBlock* bb() const
  {
    return _bb;
  }

  llvm::StringRef name() const
  {
    return _bb->getName();
  }

  uint64_t addr() const
  {
    xassert(_addr != ~0ULL && "basic block has invalid address!");
    return _addr;
  }

  /**
   * Add instruction to map.
   *
   * @param addr instruction address
   * @param instr instruction
   */
  void operator()(uint64_t addr, llvm::Instruction* instr)
  {
    _instrMap(addr, std::move(instr));
  }

  /**
   * Split basic block at the specified address.
   *
   * @param addr
   *
   * @return BasicBlock part that comes after the specified address.
   */
  BasicBlockPtr split(uint64_t addr);

  // get basic block name from the specified address.
  static std::string getBBName(uint64_t addr)
  {
    std::string name = "bb";
    llvm::raw_string_ostream ss(name);
    ss << llvm::Twine::utohexstr(addr);
    ss.flush();
    return name;
  }

private:
  Context* _ctx;
  llvm::BasicBlock* _bb;
  uint64_t _addr = ~0ULL;
  Map<uint64_t, llvm::Instruction*> _instrMap;
};

}

#endif
