#ifndef SBT_BASICBLOCK_H
#define SBT_BASICBLOCK_H

namespace sbt {

class BasicBlock
{
public:
  static std::string getBBName(uint64_t Addr)
  {
    std::string BBName = "bb";
    llvm::raw_string_ostream SS(BBName);
    SS << llvm::Twine::utohexstr(Addr);
    SS.flush();
    return BBName;
  }

  static llvm::BasicBlock* create(
    llvm::LLVMContext& context,
    uint64_t addr,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr)
  {
    return create(context, getBBName(addr), f, beforeBB);
  }

  static llvm::BasicBlock* create(
    llvm::LLVMContext& context,
    llvm::StringRef name,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB = nullptr)
  {
    DBGS << name << ":\n";
    return llvm::BasicBlock::Create(context, name, f, beforeBB);
  }

  llvm::BasicBlock *splitBB(
    llvm::BasicBlock *BB,
    uint64_t Addr);

  void updateNextBB(uint64_t Addr)
  {
    if (NextBB <= CurAddr || Addr < NextBB)
      NextBB = Addr;
  }
};

}

#endif
