#ifndef SBT_BASICBLOCK_H
#define SBT_BASICBLOCK_H

#include "Context.h"
#include "Instruction.h"
#include "Map.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

namespace sbt {

class BasicBlock
{
public:
  BasicBlock(
    Context* ctx,
    uint64_t addr,
    Function* f,
    BasicBlock* beforeBB = nullptr);

  BasicBlock(
    Context* ctx,
    uint64_t addr,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr)
    :
    BasicBlock(ctx, getBBName(addr), f, beforeBB)
  {}

  BasicBlock(
    Context* ctx,
    llvm::StringRef name,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr);

  BasicBlock(Context* ctx, llvm::BasicBlock* bb) :
    _ctx(ctx),
    _bb(bb)
  {}

  ~BasicBlock();

  BasicBlock(BasicBlock&&) = default;
  BasicBlock& operator=(BasicBlock&&) = default;

  llvm::BasicBlock* bb() const
  {
    return _bb;
  }

  llvm::StringRef name() const
  {
    return _bb->getName();
  }

  void operator()(uint64_t addr, Instruction&& instr);

  BasicBlock split(uint64_t addr);

private:
  Context* _ctx;
  llvm::BasicBlock* _bb;
  Map<uint64_t, Instruction> _instrMap;


  static std::string getBBName(uint64_t addr)
  {
    std::string name = "bb";
    llvm::raw_string_ostream ss(name);
    ss << llvm::Twine::utohexstr(addr);
    ss.flush();
    return name;
  }
};

}

#endif
