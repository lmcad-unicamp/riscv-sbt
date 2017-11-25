#include "Relocation.h"

#include "Builder.h"
#include "Instruction.h"
#include "Object.h"
#include "SBTError.h"
#include "Section.h"
#include "Translator.h"

#include <llvm/IR/Module.h>
#include <llvm/Object/ELF.h>
#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

static SBTSymbol g_invalidSymbol(0, 0, "", nullptr, 0);

SBTRelocation::SBTRelocation(
    Context* ctx,
    ConstRelocIter ri,
    ConstRelocIter re)
    :
    _ctx(ctx),
    _ri(ri),
    _re(re),
    _rlast(ri),
    _last(g_invalidSymbol)
{
}


void SBTRelocation::next(uint64_t addr, bool hadNext)
{
    // increment relocation iterator until it reaches a different address
    _rlast = _ri;
    uint64_t reladdr = hadNext? addr - Constants::INSTRUCTION_SIZE : addr;
    do {
        ++_ri;
    } while (_ri != _re && (**_ri).offset() == reladdr);
}


llvm::Expected<llvm::Value*>
SBTRelocation::handleRelocation(uint64_t addr, llvm::raw_ostream* os)
{
    // note: some relocations are used by two consecutive instructions
    bool hadNext = _next != Constants::INVALID_ADDR;
    ConstRelocationPtr reloc = nullptr;

    if (hadNext) {
        xassert(addr == _next && "unexpected relocation!");
        _next = Constants::INVALID_ADDR;
        reloc = *_ri;
    } else {
        // no more relocations exist
        if (_ri == _re)
            return nullptr;

        reloc = *_ri;

        // check if we skipped some addresses and now need to
        // advance the iterator
        if (addr > reloc->offset())
            next(addr, hadNext);

        // check if there is a relocation for current address
        if (reloc->offset() != addr)
            return nullptr;
    }

    DBGS << __METHOD_NAME__ << llvm::formatv("({0:X+8})\n", addr);

    ConstSymbolPtr sym = reloc->symbol();
    uint64_t type = reloc->type();
    bool isLO = false;
    uint64_t mask;
    ConstSymbolPtr realSym = sym;
    // note: some relocations are used by two consecutive instructions
    bool isNextToo = false;

    bool useVal = false;
    uint64_t val;
    switch (type) {
        case llvm::ELF::R_RISCV_CALL:
            if (hadNext) {
                DBGF("CALL(LO)");
                isLO = true;
            } else {
                DBGF("CALL(HI)");
                isNextToo = true;
            }
            break;

        case llvm::ELF::R_RISCV_PCREL_HI20:
            DBGF("PCREL_HI20");
            break;

        // this rellocation has PC info only
        // symbol info is present on the PCREL_HI20 reloc
        case llvm::ELF::R_RISCV_PCREL_LO12_I:
            DBGF("PCREL_LO12_I");
            isLO = true;
            realSym = (**_rlast).symbol();
            val = _last.val;
            useVal = true;
            break;

        case llvm::ELF::R_RISCV_HI20:
            DBGF("HI20");
            break;

        case llvm::ELF::R_RISCV_LO12_I:
        case llvm::ELF::R_RISCV_LO12_S:
            DBGF("LO12");
            isLO = true;
            break;

        // ignored
        case llvm::ELF::R_RISCV_ALIGN:
            DBGF("ALIGN: ignored: type={0}", type);
            next(addr, hadNext);
            _hasSymbol = false;
            return nullptr;

        case llvm::ELF::R_RISCV_BRANCH:
            DBGF("BRANCH");
            next(addr, hadNext);
            _hasSymbol = false;
            return nullptr;

        default:
            DBGF("relocation type={0}", type);
            xassert(false && "unknown relocation");
    }

    // set symbol relocation info
    SBTSymbol ssym(realSym->address(), realSym->address(),
        realSym->name(), realSym->section(), addr);
    DBGF("addr={0:X+8}, val={1:X+8}, name={2}, section={3}, addend={4:X+8}",
        ssym.addr, ssym.val, ssym.name,
        ssym.sec? ssym.sec->name() : "null",
        reloc->addend());

    xassert((ssym.sec || !ssym.addr) && "no section found for relocation");

    // XXX this finds local functions only
    bool isFunction = ssym.isFunction();

    if (isFunction) {
        if (useVal)
            ssym.val = val;
        else
            ssym.val += reloc->addend();
    } else if (ssym.sec) {
        xassert(ssym.addr < ssym.sec->size() && "out of bounds relocation");
        ssym.val += ssym.sec->shadowOffs() + reloc->addend();
    }

    if (isNextToo)
        _next = addr + Constants::INSTRUCTION_SIZE;
    else
        next(addr, hadNext);

    // write relocation string
    if (isLO) {
        mask = 0xFFF;
        *os << "%lo(";
    } else {
        mask = 0xFFFFF000;
        *os << "%hi(";
    }
    *os << ssym.name;
    if (reloc->addend())
        *os << "+" << reloc->addend();
    *os << ") = " << (ssym.addr + reloc->addend());

    llvm::Value* v = nullptr;

    // external symbol case: handle data or function
    if (ssym.isExternal()) {
        auto expAddr =_ctx->translator->import(ssym.name);

        // function not found: assume it is data
        if (!expAddr) {
            llvm::Error err = llvm::handleErrors(expAddr.takeError(),
            [](const FunctionNotFound& serr) {
                serr.log(DBGS);
            });

            if (err)
                return std::move(err);

            const Types& t = _ctx->t;
            llvm::Constant* c = _ctx->module->getOrInsertGlobal(ssym.name, t.i32);
            v = llvm::ConstantExpr::getPointerCast(c, t.i32);
            v = _ctx->bld->_and(v, _ctx->c.i32(mask));

            DBGF("external data: mask={1:X+8}", addr, mask);

        // handle external function
        } else {
            uint64_t addr = expAddr.get();
            DBGF("external function: addr={0:X+8}, mask={1:X+8}", addr, mask);

            addr &= mask;
            v = _ctx->c.i32(addr);
        }

    // internal function case
    } else if (isFunction) {
        uint64_t addr = ssym.val;
        DBGF("internal function at {0:X+8}", addr);
        addr &= mask;
        v = _ctx->c.i32(addr);

    // other relocation types:
    // add the relocation offset to ShadowImage to get the final address
    } else if (ssym.sec) {
        Builder* bld = _ctx->bld;

        // get char* to memory
        DBGF("reloc={0:X+8}", ssym.val);
        std::vector<llvm::Value*> idx = { _ctx->c.ZERO, _ctx->c.i32(ssym.val) };
        v = bld->gep(_ctx->shadowImage, idx);

        v = bld->i8PtrToI32(v);

        // get only the upper or lower part of the result
        v = bld->_and(v, _ctx->c.i32(mask));

    } else
        xassert(false && "failed to resolve relocation");

    _last = std::move(ssym);
    _hasSymbol = true;
    return v;
}


void SBTRelocation::skipRelocation(uint64_t addr)
{
    // this can't be the second part of a valid relocation
    xassert(_next == Constants::INVALID_ADDR);

    // no more relocations exist
    if (_ri == _re)
        return;

    // check if there is a relocation for current address
    ConstRelocationPtr reloc = *_ri;
    if (reloc->offset() != addr)
        return;

    // increment relocation iterator until it reaches a different address
    _last = g_invalidSymbol;
    _rlast = _ri;
    do {
        ++_ri;
    } while (_ri != _re && (**_ri).offset() == addr);
}

}
