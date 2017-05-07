#ifndef SBT_BASICBLOCK_H
#define SBT_BASICBLOCK_H

#include "Instruction.h"
#include "Map.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

namespace sbt {

class BasicBlock
{
public:
  BasicBlock(
    llvm::LLVMContext* ctx,
    uint64_t addr,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr)
    :
    BasicBlock(ctx, getBBName(addr), f, beforeBB)
  {}

  BasicBlock(
    llvm::LLVMContext* ctx,
    llvm::StringRef name,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr);


private:
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


  BasicBlock(llvm::BasicBlock* bb) :
    _bb(bb)
  {}

  BasicBlock split(uint64_t addr, llvm::IRBuilder<>* builder);

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
