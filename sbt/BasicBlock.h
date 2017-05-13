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


  BasicBlock(Context* ctx, llvm::BasicBlock* bb) :
    _ctx(ctx),
    _bb(bb)
  {}

  BasicBlock split(uint64_t addr);

  /*
  void updateNextBB(uint64_t Addr)
  {
    if (NextBB <= CurAddr || Addr < NextBB)
      NextBB = Addr;
  }
  */
};

}

#endif
