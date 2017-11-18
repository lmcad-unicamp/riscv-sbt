#include "BasicBlock.h"

#include "Builder.h"
#include "Function.h"
#include "Utils.h"

#include <llvm/IR/CFG.h>
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
    const std::string& name,
    llvm::Function* f,
    llvm::BasicBlock* beforeBB)
    :
    _ctx(ctx),
    _name(name)
{
    DBGF("{0}", _name);
    _bb = llvm::BasicBlock::Create(*_ctx->ctx, _name, f, beforeBB);
}


BasicBlock::~BasicBlock()
{
}


BasicBlockPtr BasicBlock::split(uint64_t addr)
{
    DBGF("addr={0:X+8}: 1st_addr={1:X+8}, name={2}",
        addr, this->addr(), name());

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

    llvm::BasicBlock* bb2;
    llvm::TerminatorInst* term = _bb->getTerminator();
    Builder* bld = _ctx->bld;


    // splitBasicBlock() will break if there is an empty successor
    // : insert a dummy instruction on these
    std::vector<llvm::succ_iterator> empty;
    for (llvm::succ_iterator it = llvm::succ_begin(_bb),
            end = llvm::succ_end(_bb);
            it != end; ++it)
    {
        if (it->empty()) {
            DBGF("{0} is empty. Adding dummy instr", it->getName());
            empty.push_back(it);

            bld->saveInsertBlock();
            BasicBlock tmpBB(_ctx, *it, 0);
            bld->setInsertBlock(&tmpBB);
            bld->retVoid();
            bld->restoreInsertBlock();
        }
    }

    // insert a terminator in BB if it's missing
    llvm::Instruction* instr = nullptr;
    if (!term) {
        bld->saveInsertBlock();
        bld->setInsertBlock(this);
        instr = bld->retVoid();
        bld->restoreInsertBlock();
    }

    // split
    bb2 = _bb->splitBasicBlock(i, getBBName(addr));
    // remove dummy terminator
    if (instr)
        instr->eraseFromParent();

    // remove dummy instructions from BBs
    for (auto it : empty) {
        it->getInstList().clear();
        xassert(it->empty());
    }

    DBGF("done");
    return BasicBlockPtr(new BasicBlock(_ctx, bb2, addr));
}


bool BasicBlock::terminated() const
{
    return bb()->getTerminator();
}

}
