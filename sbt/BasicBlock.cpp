#include "BasicBlock.h"

#include "Builder.h"
#include "Function.h"
#include "Utils.h"

namespace sbt {

BasicBlock::BasicBlock(
  Context* ctx,
  uint64_t addr,
  Function* f,
  BasicBlock* beforeBB)
  :
  BasicBlock(ctx, addr, f->func(), beforeBB->bb())
{}


// main constructor
BasicBlock::BasicBlock(
  Context* ctx,
  llvm::StringRef name,
  llvm::Function* f,
  llvm::BasicBlock* beforeBB)
  :
  _ctx(ctx)
{
  _bb = llvm::BasicBlock::Create(*_ctx->ctx, name, f, beforeBB);
}


BasicBlock::~BasicBlock()
{
}


BasicBlock BasicBlock::split(uint64_t addr)
{
  auto res = _instrMap[addr];
  xassert(res && "instruction not found!");
  const llvm::Instruction* tgtInstr = *res;

  // point i to target instruction
  llvm::BasicBlock::iterator i, e;
  for (i = _bb->begin(), e = _bb->end(); i != e; ++i) {
    if (&*i == tgtInstr)
      break;
  }
  xassert(i != e);

  llvm::BasicBlock *bb2;

  // case 1: basic block has terminator
  // XXX basic block already processed?
  //     no need to update insert point?
  if (_bb->getTerminator()) {
    bb2 = _bb->splitBasicBlock(i, getBBName(addr));
    return BasicBlock(_ctx, bb2);
  }

  // case 2: need to insert dummy terminator
  Builder* bld = _ctx->bld;
  xassert(bld->getInsertBlock().bb() == _bb);
  llvm::Instruction* instr = bld->retVoid();
  // split
  bb2 = _bb->splitBasicBlock(i, getBBName(addr));
  // remove dummy terminator
  instr->eraseFromParent();
  // update insert point
  _ctx->builder->SetInsertPoint(bb2, bb2->end());

  return BasicBlock(_ctx, bb2);
}

}
