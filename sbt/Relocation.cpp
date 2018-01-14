#include "Relocation.h"

#include "Builder.h"
#include "Instruction.h"
#include "Object.h"
#include "SBTError.h"
#include "Section.h"
#include "ShadowImage.h"
#include "Translator.h"

#include <llvm/IR/Module.h>
#include <llvm/Object/ELF.h>
#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

#include <functional>


namespace sbt {

static bool isLocalFunction(
    ConstSymbolPtr sym,
    ConstSectionPtr sec)
{
    // XXX this may be a bit of cheating
    if (sym && sym->type() == llvm::object::SymbolRef::ST_Function)
        return true;

    // If no symbol info is available, then consider the relocation as
    // referring to a function if it refers to the .text section.
    // (this won't work for programs that use .text to also store data)
    if (sec && sec->isText())
        return true;

    // For external functions sec will be null.
    // They are handled in other part of relocation code

    return false;
}


static bool isExternal(
    ConstSymbolPtr sym,
    ConstSectionPtr sec)
{
    return sym && sym->address() == 0 && !sec;
}


SBTRelocation::SBTRelocation(
    Context* ctx,
    ConstRelocIter ri,
    ConstRelocIter re,
    ConstSectionPtr section)
    :
    _ctx(ctx),
    _ri(ri),
    _re(re),
    _rlast(ri),
    _section(section)
{
}


void SBTRelocation::next(uint64_t addr, bool hadNext)
{
    // increment relocation iterator until it reaches a different address
    _rlast = _ri;
    // uint64_t reladdr = hadNext? addr - Constants::INSTRUCTION_SIZE : addr;
    //do {
        ++_ri;
    //} while (_ri != _re && (**_ri).offset() == reladdr);
    xassert(_ri == _re || (**_ri).offset() != addr);
}


llvm::Expected<llvm::Constant*>
SBTRelocation::handleRelocation(uint64_t addr, llvm::raw_ostream* os)
{
    // note: some relocations are used by two consecutive instructions
    /*
    bool hadNext = _next != Constants::INVALID_ADDR;
    xassert(!hadNext);
    */
    bool hadNext = false;

    ConstRelocationPtr reloc = nullptr;

    /*
    if (hadNext) {
        xassert(addr == _next && "unexpected relocation!");
        _next = Constants::INVALID_ADDR;
        reloc = *_ri;
    } else {
    */
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

        next(addr, hadNext);
    //}

    DBGS << __METHOD_NAME__ << llvm::formatv("({0:X+8})\n", addr);

    // NOTE sym may be null for section relocation
    ConstSymbolPtr sym = reloc->symbol();
    // NOTE sec may be null for external symbol relocation
    ConstSectionPtr sec = reloc->section();

    DBGF("reloc: offs={0:X+8}, type={1}, symbol=\"{2}\", section=\"{3}\", "
            "addend={4:X+8}",
        reloc->offset(),
        reloc->typeName(),
        sym? sym->name() : "<null>",
        sec? sec->name() : "<null>",
        reloc->addend());

    /*
    ConstSymbolPtr realSym = sym;
    bool isLO = false;
    uint64_t mask;
    bool useVal = false;
    uint64_t val;
    // note: some relocations are used by two consecutive instructions
    bool isNextToo = false;
    */

    std::function<llvm::Constant*(llvm::Constant* addr)> relfn;

    switch (reloc->type()) {
        /*
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
        */

        case llvm::ELF::R_RISCV_HI20:
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* c =
                    llvm::ConstantExpr::getAnd(
                        addr,
                        _ctx->c.i32(0xFFFFF000));
                return c;
            };
            break;

        case llvm::ELF::R_RISCV_LO12_I:
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* c =
                    llvm::ConstantExpr::getAnd(
                        addr,
                        _ctx->c.i32(0xFFF));
                return c;
            };
            break;

        /*
        case llvm::ELF::R_RISCV_LO12_S:
            DBGF("LO12");
            isLO = true;
            break;

        case llvm::ELF::R_RISCV_JAL:
            DBGF("JAL");
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
        */

        default:
            xassert(false && "unknown relocation");
    }

    /*
    // set symbol relocation info
    uint64_t realSymAddr = 0;
    uint64_t realSymVal = 0;
    std::string realSymName = "noname";
    ConstSectionPtr realSymSec = nullptr;

    if (realSym) {
        realSymAddr = realSym->address();
        realSymVal = realSymAddr;
        realSymName = realSym->name();
        realSymSec = realSym->section();
    }

    SBTSymbol ssym(realSymAddr, realSymVal, realSymName,
        realSymSec, addr);
    DBGF("addr={0:X+8}, val={1:X+8}, name={2}, section={3}, addend={4:X+8}",
        ssym.addr, ssym.val, ssym.name,
        ssym.sec? ssym.sec->name() : "null",
        reloc->addend());

    xassert((ssym.sec || !ssym.addr) && "no section found for relocation");
    */

    bool isLocalFunc = isLocalFunction(sym, sec);
    bool isExt = isExternal(sym, sec);

    // we need the symbol for external references
    xassert(!isExt || sym &&
            "external symbol relocation but symbol not found");

    // bounds check for non-external symbols
    xassert(!(sym && !isExt) ||
            sym->address() < sec->size() &&
            "out of bounds symbol relocation");


    /*
    if (isNextToo)
        _next = addr + Constants::INSTRUCTION_SIZE;
    else
        next(addr, hadNext);
    */

    llvm::Constant* c = nullptr;

    // external symbol case: handle data or function
    if (isExt) {
        auto expAddr =_ctx->translator->import(sym->name());
        if (!expAddr)
            return expAddr.takeError();
        uint64_t saddr = expAddr.get();

        // data
        if (saddr == SYM_TYPE_DATA) {
            DBGF("external data: saddr={0:X+8}", saddr);

            const Types& t = _ctx->t;
            c = _ctx->module->getOrInsertGlobal(sym->name(), t.i32);
            c = llvm::ConstantExpr::getPointerCast(c, t.i32);

        // handle external function
        } else {
            DBGF("external function: saddr={0:X+8}", saddr);
            c = _ctx->c.i32(saddr);
        }

        if (os)
            *os << sym->name();

    // internal function case
    } else if (isLocalFunc) {
        xassert(sec);

        uint64_t saddr;

        // get address
        if (sym)
            saddr = sym->address();
        else
            saddr = reloc->addend();

        // get function
        Function* f = Function::getByAddr(_ctx, saddr);

        DBGF("internal function: name=\"{0}\", saddr={1:X+8}",
            f->name(), saddr);

        // XXX for now, internal functions are always called
        //     indirectly, via icaller, so return just their
        //     original guest address
        c = _ctx->c.i32(saddr);

        if (os)
            *os << f->name();

    // other relocations
    // add the relocation offset to ShadowImage to get the final address
    } else {
        xassert(sec);
        uint64_t saddr;

        if (sym) {
            DBGF("symbol reloc: symbol=\"{0}\", addend={1:X+8}, section=\"{2}\"",
                sym->name(),
                reloc->addend(),
                sec->name());

            saddr = sym->address() + reloc->addend();

            if (os)
                *os << sym->name();

        } else {
            DBGF("section reloc: section=\"{0}\", addend={1:X+8}",
                sec->name(),
                reloc->addend());

            saddr = reloc->addend();

            if (os)
                *os << llvm::formatv("{0}+{1:X+8}", sec->name(), reloc->addend());
        }

        // TODO speed up section lookup for relocations
        c = llvm::ConstantExpr::getAdd(
            _ctx->shadowImage->getSection(sec->name()),
            _ctx->c.i32(saddr));
    }

    c = relfn(c);
    c->dump();
    return c;
}


void SBTRelocation::skipRelocation(uint64_t addr)
{
    // this can't be the second part of a valid relocation
    // xassert(_next == Constants::INVALID_ADDR);

    // no more relocations exist
    if (_ri == _re)
        return;

    // check if there is a relocation for current address
    ConstRelocationPtr reloc = *_ri;
    if (reloc->offset() != addr)
        return;

    // increment relocation iterator until it reaches a different address
    _rlast = _ri;
    do {
        ++_ri;
    } while (_ri != _re && (**_ri).offset() == addr);
}


llvm::GlobalVariable* SBTRelocation::relocateSection(
    const std::vector<uint8_t>& bytes,
    const ShadowImage* shadowImage)
{
    xassert(_section);
    DBGF("relocating section {0}", _section->name());
    std::vector<llvm::Constant*> cvec;

    size_t n = bytes.size();
    ConstRelocIter rit = _ri;

    for (size_t addr = 0; addr < n; addr+=4) {
        // reloc
        if (rit != _re && addr == (*rit)->offset()) {
            ConstRelocationPtr reloc = *rit;
            switch (reloc->type()) {
                case llvm::ELF::R_RISCV_32: {
                    ConstSymbolPtr sym = reloc->symbol();
                    xassert(sym);
                    ConstSectionPtr symSec = reloc->section();
                    xassert(symSec);
                    DBGF(
                        "relocating {0:X-8}: "
                        "symbol: name={1}, section={2}, offs={3:X-8}",
                        reloc->offset(),
                        sym->name(), symSec->name(), sym->address());

                    llvm::Constant* cexpr =
                        llvm::ConstantExpr::getAdd(
                            shadowImage->getSection(symSec->name()),
                            _ctx->c.u32(sym->address()));
                    cvec.push_back(cexpr);
                    break;
                }

                default:
                    DBGF("unknown relocation type: {0}", reloc->type());
                    xunreachable("Unsupported relocation type");
            }
            ++rit;
        // or just copy the raw bytes
        } else {
            uint32_t val = 0;
            for (int i = 3; i >= 0; i--) {
                val |= bytes[addr + i];
                val <<= 8;
            }
            DBGF("copying raw bytes @{0:X-8}: {1:X-8}", addr, val);
            cvec.push_back(_ctx->c.u32(val));
        }
    }

    llvm::ArrayType* aty = llvm::ArrayType::get(_ctx->t.i32, cvec.size());
    llvm::Constant* ca = llvm::ConstantArray::get(aty, cvec);
    return new llvm::GlobalVariable(*_ctx->module, ca->getType(),
        !CONSTANT, llvm::GlobalValue::ExternalLinkage, ca);
}

}
