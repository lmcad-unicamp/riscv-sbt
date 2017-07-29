#ifndef SBT_FUNCTION_H
#define SBT_FUNCTION_H

#include "BasicBlock.h"
#include "Context.h"
#include "Map.h"
#include "Object.h"
#include "Pointer.h"
#include "XRegisters.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <memory>


namespace llvm {
class BasicBlock;
class Function;
class Instruction;
class Module;
}

namespace sbt {

class SBTSection;

class Function
{
public:
    /**
     * Construct instruction.
     *
     * @param ctx
     * @param name
     * @param sec section of translated binary that contains the function
     * @param addr function address
     * @param end guest memory address of first byte after function's last byte
     *
     * Note: section/end may be null/0 when the function does not correspond
     *             to a guest one.
     */
    Function(
        Context* ctx,
        const std::string& name,
        SBTSection* sec = nullptr,
        uint64_t addr = Constants::INVALID_ADDR,
        uint64_t end = Constants::INVALID_ADDR)
        :
        _ctx(ctx),
        _name(name),
        _sec(sec),
        _addr(addr),
        _end(end)
    {}

    /**
     * Create the function.
     *
     * @param ft function type
     * @param linkage function linkage
     */
    void create(
        llvm::FunctionType* ft = nullptr,
        llvm::Function::LinkageTypes linkage = llvm::Function::ExternalLinkage);

    // translate function
    llvm::Error translate();


    // getters

    const std::string& name() const
    {
        return _name;
    }

    // llvm function pointer
    llvm::Function* func() const
    {
        return _f;
    }

    uint64_t addr() const
    {
        return _addr;
    }

    //

    // BB

    BasicBlock* newBB(uint64_t addr, BasicBlock* beforeBB = nullptr)
    {
        _bbMap(addr, BasicBlockPtr(
            new BasicBlock(_ctx, addr, _f,
                beforeBB? beforeBB->bb() : nullptr)));
        return &**_bbMap[addr];
    }

    BasicBlock* addBB(uint64_t addr, BasicBlockPtr&& bb)
    {
        _bbMap(addr, std::move(bb));
        return &**_bbMap[addr];
    }

    BasicBlock* newUBB(uint64_t addr, const std::string& name)
    {
        const std::string bbname = BasicBlock::getBBName(addr) + "_" + name;
        BasicBlock* bb = new BasicBlock(_ctx, bbname, _f);
        BasicBlockPtr bbptr(bb);

        auto* p = _ubbMap[addr];
        // first on this addr
        if (!p)
            _ubbMap(addr, std::vector<BasicBlockPtr>(std::move(bbptr)));
        else
            p->emplace_back(std::move(bbptr));
        return bb;
    }

    BasicBlock* findBB(uint64_t addr)
    {
        BasicBlockPtr* ptr = _bbMap[addr];
        if (!ptr)
            return nullptr;
        return &**ptr;
    }

    BasicBlock* lowerBoundBB(uint64_t addr)
    {
        auto it = _bbMap.lower_bound(addr);
        if (it == _bbMap.end())
            return nullptr;
        return &*it->val;
    }

    BasicBlock* getBackBB(uint64_t addr)
    {
        auto it = _bbMap.lower_bound(addr);

        // last
        if (it == _bbMap.end()) {
            xassert(!_bbMap.empty() && "empty bbMap!");
            --it;
        // exact match
        } else if (it->key == addr)
            ;
        // previous
        else if (it != _bbMap.begin())
            --it;
        // else: first

        return &*it->val;
    }

    uint64_t nextBBAddr(uint64_t curAddr)
    {
        auto it = _bbMap.lower_bound(curAddr + Constants::INSTRUCTION_SIZE);
        if (it != _bbMap.end())
            return it->key;
        return Constants::INVALID_ADDR;
    }

    // basic block map
    Map<uint64_t, BasicBlockPtr>& bbmap()
    {
        return _bbMap;
    }


    // update next basic block address
    void updateNextBB(uint64_t addr)
    {
        if (_nextBB <= addr || addr < _nextBB)
            _nextBB = addr;
    }

    // BB end

    void setEnd(uint64_t end)
    {
        _end = end;
    }

    void transferBBs(uint64_t from, Function* to);

    /**
     * Translate instructions at given address range.
     *
     * @param st starting address
     * @param end end address (after last instruction)
     */
    llvm::Error translateInstrs(uint64_t st, uint64_t end);

    // was function terminated?
    bool terminated() const;

    XRegister& getReg(size_t i)
    {
        xassert(_regs);
        return _regs->getReg(i);
    }

    void cleanRegs();

    // sync local register file
    void loadRegisters();
    void storeRegisters();

    void copyArgv();

    // look up function by address
    static Function* getByAddr(Context* ctx, uint64_t addr);

private:
    Context* _ctx;
    std::string _name;
    SBTSection* _sec;
    uint64_t _addr;
    uint64_t _end;

    llvm::Function* _f = nullptr;
    // address of next basic block
    uint64_t _nextBB = 0;
    Map<uint64_t, BasicBlockPtr> _bbMap;

    // untracked BBs map: <addr, BBs>
    // addr: guest address where the BBs belong to
    Map<uint64_t, std::vector<BasicBlockPtr>> _ubbMap;

    Function* _nextf = nullptr;

    std::unique_ptr<XRegisters> _regs;

    // methods

    llvm::Error startMain();
    llvm::Error start();
    llvm::Error finish();

    // current basic block pointer
    BasicBlock* curBB();
};

using FunctionPtr = Pointer<Function>;

}

#endif
