#include "Function.h"

#include "Builder.h"
#include "Constants.h"
#include "Instruction.h"
#include "SBTError.h"
#include "Section.h"
#include "Stack.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

void Function::create(
    llvm::FunctionType* ft,
    llvm::Function::LinkageTypes linkage)
{
    // if function already exists in LLVM module, do nothing
    _f = _ctx->module->getFunction(_name);
    if (_f)
        return;

    DBGF("{0}", _name);

    // create a function with no parameters if no function type was specified
    if (!ft)
        ft = _ctx->t.voidFunc;
    _f = llvm::Function::Create(ft, linkage, _name, _ctx->module);
}


llvm::Error Function::translate()
{
    DBGS << _name << ":\n";

    xassert(_sec && "null section pointer!");
    xassert(_addr != Constants::INVALID_ADDR);
    xassert(_end != Constants::INVALID_ADDR);

    // start
    if (_name == "main") {
        if (auto err = startMain())
            return err;
    } else {
        if (auto err = start())
            return err;
    }

    // translate instructions
    if (auto err = translateInstrs(_addr, _end))
        return err;

    // finish
    if (auto err = finish())
        return err;

    // finished earlier: switch to next function
    if (_nextf) {
        // transfer unfinished BBs to new function
        transferBBs(_end, _nextf);
        return _sec->translate(_nextf);
    }

    return llvm::Error::success();
}


llvm::Error Function::startMain()
{
    const Types& t = _ctx->t;
    Builder* bld = _ctx->bld;

    // int main();
    llvm::FunctionType* ft =
        llvm::FunctionType::get(t.i32,
            { t.i32, t.i32 },
            !VAR_ARG);
    _f = llvm::Function::Create(ft,
        llvm::Function::ExternalLinkage, _name, _ctx->module);

    // first bb
    bld->setInsertBlock(newBB(_addr));

    // create local register file
    if (_localRegs) {
        _regs.reset(new XRegisters(_ctx, XRegisters::LOCAL));
        _fregs.reset(new FRegisters(_ctx, FRegisters::LOCAL));
    }

    copyArgv();

    // set stack pointer
    bld->store(_ctx->stack->end(), XRegister::SP);

    _ctx->inMain = true;
    return llvm::Error::success();
}


llvm::Error Function::start()
{
    create();

    DBGF("addr={0:X+8}", _addr);

    BasicBlock* ptr = findBB(_addr);
    if (!ptr)
        ptr = newBB(_addr);
    _ctx->bld->setInsertBlock(ptr);

    // create local register file
    if (_localRegs) {
        _regs.reset(new XRegisters(_ctx, XRegisters::LOCAL));
        _fregs.reset(new FRegisters(_ctx, FRegisters::LOCAL));
    }
    loadRegisters();

    return llvm::Error::success();
}


bool Function::terminated() const
{
    llvm::Value* v = _ctx->bld->getInsertBlock()->bb()->getTerminator();
    if (v == nullptr)
        return false;
    return llvm::dyn_cast<llvm::ReturnInst>(v);
}


llvm::Error Function::finish()
{
    auto bld = _ctx->bld;
    // add a return if there is no terminator in current block
    if (!bld->getInsertBlock()->terminated()) {
        // there must always be a return instruction in main,
        // except when a function that does not return is used,
        // such as exit().
        // In this case, the next instruction will be unreachable.
        if (_ctx->inMain)
            bld->unreachable();
        else {
            storeRegisters();
            bld->retVoid();
        }
    }
    _ctx->inMain = false;
    cleanRegs();

    // last BB may be empty
    auto it = _bbMap.end();
    --it;
    if (it->val->bb()->empty()) {
        DBGF("removing empty BB: {0}", it->val->name());
        it->val->bb()->eraseFromParent();
    }

    // _f->dump();
    // _f->viewCFG();
    return llvm::Error::success();
}


void Function::cleanRegs()
{
    if (!_localRegs)
        return;

    auto cleanReg = [](Register& r) {
        if (!r.hasAccess()) {
            llvm::Value* v = r.get();
            llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(v);
            xassert(inst);

            while (!inst->use_empty()) {
                llvm::Instruction* use = inst->user_back();
                // These are assigning a value to the local copy of the reg,
                // but since we don't use it, we can delete the assignment.
                if (llvm::isa<llvm::StoreInst>(use)) {
                    use->eraseFromParent();
                    continue;
                }

                xassert(llvm::isa<llvm::LoadInst>(use));
                // Here we should have a false usage of the value.
                // It is loading only in checkpoints (exit points) to
                // save it back to the global copy.
                // Since we do not really use it, we should delete the
                // load and the store insruction that is using it.
                xassert(use->hasOneUse());
                llvm::Instruction *use2 =
                    llvm::dyn_cast<llvm::Instruction>(use->user_back());
                xassert(llvm::isa<llvm::StoreInst>(use2));
                use2->eraseFromParent();
                use->eraseFromParent();
            }
            inst->eraseFromParent();
        }
    };

    for (size_t i = 1; i < XRegisters::NUM; i++)
        cleanReg(_regs->getReg(i));
    for (size_t i = 0; i < FRegisters::NUM; i++)
        cleanReg(_fregs->getReg(i));
}


llvm::Error Function::translateInstrs(uint64_t st, uint64_t end)
{
    DBGF("({0:X+8}, {1:X+8})", st, end);

    ConstSectionPtr section = _sec->section();
    const llvm::ArrayRef<uint8_t> bytes = _sec->bytes();

    // for each instruction
    // XXX assuming fixed size instructions
    uint64_t size = Constants::INSTRUCTION_SIZE;
    Builder* bld = _ctx->bld;
    for (uint64_t addr = st; addr < end; addr += size) {
        _ctx->addr = addr;

        // get raw instruction
        const uint8_t* rawBytes = &bytes[addr];
        uint32_t rawInst = *reinterpret_cast<const uint32_t*>(rawBytes);

        // check if we need to switch to a new function
        // (calling a target with no global/function symbol info
        //    introduces a new function)
        // (but don't switch to self)
        if (addr != st && addr == _sec->getNextFuncAddr()) {
            _nextf = _ctx->funcByAddr(addr);
            DBGF("switching to function {0} at address {1:X+8}", _nextf->name(), addr);

            _nextf->setEnd(end);
            _end = addr;

            return llvm::Error::success();
        }

        // update current bb
        DBGF("addr={0:X+8}, _nextBB={1:X+8}", addr, _nextBB);
        if (_nextBB && addr == _nextBB) {
            BasicBlock* bbptr = findBB(addr);
            if (!bbptr)
                bbptr = newBB(addr, lowerBoundBB(addr));
            DBGF("insertPoint={0}:", bbptr->name());

            // if last instruction did not terminate the BB,
            // then branch from previous BB to current one
            if (!bld->getInsertBlock()->terminated())
                bld->br(bbptr);
            bld->setInsertBlock(bbptr);

            // set next BB pointer
            uint64_t nextBB = nextBBAddr(addr);
            if (nextBB != Constants::INVALID_ADDR)
                updateNextBB(nextBB);
        }

        // translate instruction
        Instruction inst(_ctx, addr, rawInst);
        BasicBlock* bb = bld->getInsertBlock();
        if (auto err = inst.translate())
            return err;
        // add translated instruction to BB's instruction map
        llvm::Instruction* first = bld->first();
        llvm::BasicBlock* fbb = first->getParent();

        // BB of first instr != BB before translating instr:
        //   - BB was split
        //   - current BB became the bottom part of the split
        //   - first instruction should be on the bottom part
        if (fbb != bb->bb()) {
            bb = bld->getInsertBlock();
            xassert(!bb->untracked());
            xassert(bb->bb() == fbb);
        }

        bb->addInstr(addr, std::move(first));

        // If last instruction terminated this basicblock
        // switch to a new one. This is needed in case the
        // next basic block was not explicitly created.
        // This can happen, for instance, if an unconditional
        // jump is followed by alignment instrs.
        //
        // Also switch to a new BB if we are inside an
        // untracked BB, to make it possible to split
        // the BB later.
        bb = bld->getInsertBlock();
        if (bb->terminated() || bb->untracked())
            updateNextBB(addr + Constants::INSTRUCTION_SIZE);
    }

    return llvm::Error::success();
}


static int isFunction(ConstSymbolPtrVec symv)
{
    int i = 1;
    for (const auto& sym : symv) {
        DBGF("{0}: sym={1}, ST_Function={2}, SF_Global={3}: {4}",
            i, !!sym,
            sym? (sym->type() == llvm::object::SymbolRef::ST_Function) : false,
            sym? (sym->flags() & llvm::object::SymbolRef::SF_Global) : false,
            sym? sym->str() : "");

        if (sym &&
            ((sym->type() == llvm::object::SymbolRef::ST_Function) ||
            (sym->flags() & llvm::object::SymbolRef::SF_Global)))
            return i;
        i++;
    }
    return 0;
}


bool Function::isFunction(Context* ctx, uint64_t addr)
{
    if (auto fp = ctx->funcByAddr(addr, !ASSERT_NOT_NULL))
        return true;

    // get symbol by offset
    ConstSymbolPtrVec symv = ctx->sec->section()->lookup(addr);
    return sbt::isFunction(symv);
}


Function* Function::getByAddr(Context* ctx, uint64_t addr)
{
    if (auto fp = ctx->funcByAddr(addr, !ASSERT_NOT_NULL))
        return fp;

    // get symbol by offset
    ConstSymbolPtrVec symv = ctx->sec->section()->lookup(addr);
    // XXX lookup by symbol name
    if (symv.empty())
        DBGF("WARNING: symbol not found at {0:X+8}", addr);

    // create a new function
    std::string name;
    int n;
    if (!(n=sbt::isFunction(symv)))
        name = "f" + llvm::Twine::utohexstr(addr).str();
    else
        name = symv.at(--n)->name();
    FunctionPtr f(new Function(ctx, name, ctx->sec, addr));
    f->create();
    // insert in maps
    ctx->addFunc(std::move(f));

    ctx->sec->updateNextFuncAddr(addr);

    return ctx->funcByName(name);
}


void Function::transferBBs(uint64_t from, Function* to)
{
    DBGF("from={0:X+8}, to={1}, func={2}", from, to->name(), name());

    // transfer tracked BBs
    to->_nextBB = 0;
    auto st = _bbMap.lower_bound(from);
    auto it = st;
    auto end = _bbMap.end();
    xassert(to->_bbMap.empty());

    while (it != end) {
        auto key = it->key;
        DBGF("key={0:X+8}", key);
        to->_bbMap.upsert(key, std::move(it->val));

        DBG(BasicBlockPtr* val = to->_bbMap[key];
            xassert(val);
            DBGF("{0}", (*val)->name()));

        if (!to->_nextBB && key != from)
            to->_nextBB = key;

        ++it;
    }
    DBGF("erasing our bbMap...");
    _bbMap.erase(st);
    DBGF("bbMap done");

    // transfer untracked BBs
    DBGF("tranferring uBBs...");
    auto ust = _ubbMap.lower_bound(from);
    auto uit = ust;
    auto uend = _ubbMap.end();
    xassert(to->_ubbMap.empty());

    while (uit != uend) {
        auto ukey = uit->key;
        DBGF("ukey={0:X+8}", ukey);
        to->_ubbMap.upsert(ukey, std::move(uit->val));
        ++uit;
    }
    DBGF("erasing our ubbMap...");
    _ubbMap.erase(ust);
    DBGF("ubbMap done");
}


void Function::loadRegisters(bool retRegsOnly)
{
    if (!_localRegs)
        return;

    Builder* bld = _ctx->bld;
    xassert(bld);

    auto loadXReg = [&](size_t i) {
        Register& local = getReg(i);
        Register& global = _ctx->x->getReg(i);
        // NOTE don't count this write
        llvm::Value* v = bld->load(global.getForRead());
        bld->store(v, local.get());
    };

    if (retRegsOnly) {
        for (size_t i = XRegister::A0; i <= XRegister::A1; i++)
            loadXReg(i);
        return;
    }

    for (size_t i = 1; i < XRegisters::NUM; i++)
        loadXReg(i);

    if (!_ctx->opts->syncFRegs())
        return;

    for (size_t i = 0; i < FRegisters::NUM; i++) {
        Register& local = getFReg(i);
        Register& global = _ctx->f->getReg(i);
        // NOTE don't count this write
        llvm::Value* v = bld->load(global.getForRead());
        bld->store(v, local.get());
    }
}


void Function::storeRegisters()
{
    if (!_localRegs)
        return;

    Builder* bld = _ctx->bld;
    xassert(bld);

    for (size_t i = 1; i < XRegisters::NUM; i++) {
        Register& local = getReg(i);
        Register& global = _ctx->x->getReg(i);
        // NOTE don't count this read
        llvm::Value* v = bld->load(local.get());
        bld->store(v, global.getForWrite());
    }

    if (!_ctx->opts->syncFRegs())
        return;

    for (size_t i = 0; i < FRegisters::NUM; i++) {
        Register& local = getFReg(i);
        Register& global = _ctx->f->getReg(i);
        // NOTE don't count this read
        llvm::Value* v = bld->load(local.get());
        bld->store(v, global.getForWrite());
    }
}


void Function::copyArgv()
{
    llvm::Function::arg_iterator arg = _f->arg_begin();
    llvm::Argument& argc = *arg;
    llvm::Argument& argv = *++arg;

    Builder* bld = _ctx->bld;
    bld->store(&argc, XRegister::A0);
    bld->store(&argv, XRegister::A1);
}

}
