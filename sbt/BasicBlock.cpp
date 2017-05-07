#include "BasicBlock.h"

#include "Builder.h"
#include "Utils.h"

namespace sbt {

BasicBlock::BasicBlock(
  llvm::LLVMContext* ctx,
  llvm::StringRef name,
  llvm::Function* f,
  llvm::BasicBlock* beforeBB)
{
  DBGS << name << ":\n";
  _bb = llvm::BasicBlock::Create(*ctx, name, f, beforeBB);
}


BasicBlock BasicBlock::split(uint64_t addr, llvm::IRBuilder<>* builder)
{
  auto res = _instrMap[addr];
  assert(res && "Instruction not found!");

  const llvm::Instruction* tgtInstr = res->instr();

  llvm::BasicBlock::iterator i, e;
  for (i = _bb->begin(), e = _bb->end(); i != e; ++i) {
    if (&*i == tgtInstr)
      break;
  }
  xassert(i != e);

  llvm::BasicBlock *bb2;
  if (_bb->getTerminator()) {
    bb2 = _bb->splitBasicBlock(i, getBBName(addr));
    // XXX BBMap(addr, std::move(bb2));
    return BasicBlock(bb2);
  }

  // Insert dummy terminator
  xassert(builder->GetInsertBlock() == _bb);
  Builder bld(builder);
  llvm::Instruction* instr = bld.retVoid();
  bb2 = _bb->splitBasicBlock(i, getBBName(addr));
  // XXX BBMap(Addr, std::move(BB2));
  instr->eraseFromParent();
  builder->SetInsertPoint(bb2, bb2->end());

  return BasicBlock(bb2);
}

}
