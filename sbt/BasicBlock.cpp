#include "BasicBlock.h"

#include "Builder.h"
#include "Function.h"
#include "Utils.h"

#include <llvm/Support/FormatVariadic.h>

namespace sbt {

BasicBlock::BasicBlock(
    Context* ctx,
    uint64_t addr,
    Function* f,
    BasicBlock* beforeBB)
    :
    BasicBlock(ctx, addr, f->func(), beforeBB? beforeBB->bb() : nullptr)
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
    DBGF("{0}", name);
    _bb = llvm::BasicBlock::Create(*_ctx->ctx, name, f, beforeBB);
}


BasicBlock::~BasicBlock()
{
}


BasicBlockPtr BasicBlock::split(uint64_t addr)
{
    DBGF("addr={0:X+8}: cur={1:X+8}", addr, this->addr());

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
    //         no need to update insert point?
    if (_bb->getTerminator()) {
        bb2 = _bb->splitBasicBlock(i, getBBName(addr));
        return BasicBlockPtr(new BasicBlock(_ctx, bb2, addr));
    }

    // case 2: need to insert dummy terminator
    Builder* bld = _ctx->bld;
    xassert(bld->getInsertBlock()->bb() == _bb);
    llvm::Instruction* instr = bld->retVoid();
    // split
    bb2 = _bb->splitBasicBlock(i, getBBName(addr));
    // remove dummy terminator
    instr->eraseFromParent();

    return BasicBlockPtr(new BasicBlock(_ctx, bb2, addr));
}

}
