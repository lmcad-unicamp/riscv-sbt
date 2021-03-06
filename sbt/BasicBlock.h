#ifndef SBT_BASICBLOCK_H
#define SBT_BASICBLOCK_H

#include "Context.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

#include <map>

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
        const std::string& name,
        llvm::Function* f,
        llvm::BasicBlock* beforeBB = nullptr);

    /**
     * Construct from existing llvm::BasicBlock
     */
    BasicBlock(
        Context* ctx,
        llvm::BasicBlock* bb,
        uint64_t addr,
        std::map<uint64_t, llvm::Instruction*>&& instrMap)
        :
        _ctx(ctx),
        _bb(bb),
        _name(bb->getName()),
        _addr(addr),
        _instrMap(std::move(instrMap))
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

    const std::string& name() const
    {
        return _name;
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
    void addInstr(uint64_t addr, llvm::Instruction* instr)
    {
        _instrMap[addr] = instr;
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

    /**
     * Has this BB been terminated already?
     */
    bool terminated() const;

    /**
     * Do we care about this BB? Or is it used just to hold only auxiliary
     * code?
     */
    bool untracked() const
    {
        return _untracked;
    }

    /**
     * Set/reset untracked flag.
     */
    void untracked(bool v)
    {
        _untracked = v;
    }

private:
    Context* _ctx;
    llvm::BasicBlock* _bb;
    std::string _name;
    uint64_t _addr = ~0ULL;
    std::map<uint64_t, llvm::Instruction*> _instrMap;
    bool _untracked = false;
};

}

#endif
