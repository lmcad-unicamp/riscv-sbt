#include "BasicBlock.h"

#include "Builder.h"
#include "Utils.h"

namespace sbt {

BasicBlock::BasicBlock(
  Context* ctx,
  llvm::StringRef name,
  llvm::Function* f,
  llvm::BasicBlock* beforeBB)
  :
  _ctx(ctx)
{
  DBGS << name << ":\n";
  _bb = llvm::BasicBlock::Create(*_ctx->ctx, name, f, beforeBB);
}


BasicBlock::~BasicBlock()
{
}


BasicBlock BasicBlock::split(uint64_t addr)
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
    return BasicBlock(_ctx, bb2);
  }

  // Insert dummy terminator
  xassert(_ctx->builder->GetInsertBlock() == _bb);
  Builder bld(_ctx);
  llvm::Instruction* instr = bld.retVoid();
  bb2 = _bb->splitBasicBlock(i, getBBName(addr));
  // XXX BBMap(Addr, std::move(BB2));
  instr->eraseFromParent();
  _ctx->builder->SetInsertPoint(bb2, bb2->end());

  return BasicBlock(_ctx, bb2);
}


void BasicBlock::operator()(uint64_t addr, Instruction&& instr)
{
  _instrMap(addr, std::move(instr));
}

}
