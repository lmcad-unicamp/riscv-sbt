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
    _section(section)
{
}


void SBTRelocation::next(uint64_t addr)
{
    ++_ri;
    xassert(_ri == _re || (**_ri).offset() != addr);
}


llvm::Expected<llvm::Constant*>
SBTRelocation::handleRelocation(uint64_t addr, llvm::raw_ostream* os)
{
    ConstRelocationPtr reloc = nullptr;

    // no more relocations exist
    if (_ri == _re)
        return nullptr;

    reloc = *_ri;

    // check if we skipped some addresses and now need to
    // advance the iterator
    if (addr > reloc->offset())
        next(addr);

    // check if there is a relocation for current address
    if (reloc->offset() != addr)
        return nullptr;

    next(addr);

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

    std::function<llvm::Constant*(llvm::Constant* addr)> relfn;

    switch (reloc->type()) {
        case llvm::ELF::R_RISCV_HI20:
            // addr >>= 12
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* c =
                    llvm::ConstantExpr::getLShr(
                        addr,
                        _ctx->c.i32(12));
                return c;
            };
            break;

        case llvm::ELF::R_RISCV_LO12_I:
        case llvm::ELF::R_RISCV_LO12_S:
            // addr &= 0xFFF
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* c =
                    llvm::ConstantExpr::getAnd(
                        addr,
                        _ctx->c.i32(0xFFF));
                return c;
            };
            break;

        default:
            xassert(false && "unknown relocation");
    }

    bool isLocalFunc = isLocalFunction(sym, sec);
    bool isExt = isExternal(sym, sec);

    // we need the symbol for external references
    xassert(!isExt || sym &&
            "external symbol relocation but symbol not found");

    // bounds check for non-external symbols
    xassert(!(sym && !isExt) ||
            sym->address() < sec->size() &&
            "out of bounds symbol relocation");

    llvm::Constant* c = nullptr;

    // external symbol case: handle data or function
    if (isExt) {
        auto expAddr =_ctx->translator->import(sym->name());
        if (!expAddr)
            return expAddr.takeError();
        uint64_t saddr = expAddr.get().first;
        const std::string& xfunc = expAddr.get().second;

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
            *os << xfunc;

    // internal function or label case
    // FIXME this part is still a best effort to try to find out if
    // the relocation refers to a function or a label
    } else if (isLocalFunc) {
        xassert(sec);

        uint64_t saddr;
        bool isFunc;

        // get address
        if (sym) {
            saddr = sym->address();
            // if there is a symbol, this is most likely a function
            isFunc = true;
        } else {
            saddr = reloc->addend();
            isFunc = false;
        }

        // check for existing functions
        Function* f = _ctx->funcByAddr(saddr, !ASSERT_NOT_NULL);
        if (f)
            isFunc = true;

        // no relocation symbol found: try to find a section symbol
        if (!isFunc)
            isFunc = !!_ctx->sec->section()->lookup(addr);

        if (isFunc && !f) {
            // function not found: create one only if it's ahead of
            // current translation address
            if (saddr > _ctx->addr)
                f = Function::getByAddr(_ctx, saddr);
            // Otherwise, currently the SBT does not create a new function for
            // an already translated code, so the best we can do for now is to
            // just return the address of where the function would be located.
            // If the function is never called, there will be no problem.
            // Otherwise, either the SBT will abort when trying to call a
            // non-existing function or a new function with an unique name
            // will be declared but never defined, resulting in a link error
            // later.
        }

        if (f) {
            DBGF("internal function: name=\"{0}\", saddr={1:X+8}",
                f->name(), saddr);
            if (os)
                *os << f->name();
        } else {
            DBGF("internal label: saddr={0:X+8}", saddr);
            if (os)
                *os << llvm::formatv("{0:X+8}", saddr);
        }

        // XXX for now, internal functions are always called
        //     indirectly, via icaller, so return just their
        //     original guest address
        c = _ctx->c.i32(saddr);


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
    return c;
}


void SBTRelocation::skipRelocation(uint64_t addr)
{
    // no more relocations exist
    if (_ri == _re)
        return;

    // check if there is a relocation for current address
    ConstRelocationPtr reloc = *_ri;
    if (reloc->offset() != addr)
        return;

    next(addr);
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
                    uint64_t addr = reloc->addend();
                    ConstSectionPtr sec = reloc->section();
                    xassert(sec);
                    if (sym) {
                        addr += sym->address();
                        DBGF(
                            "relocating {0:X-8}: "
                            "symbol=\"{1}\", symaddr={2:X+8}, "
                            "section=\"{3}\", addend={4:X+8}, "
                            "addr={5:X-8}",
                            reloc->offset(),
                            sym->name(), sym->address(),
                            sec->name(), reloc->addend(),
                            addr);
                    } else {
                        DBGF(
                            "relocating {0:X-8}: "
                            "section=\"{1}\", addend={2:X+8}",
                            reloc->offset(),
                            sec->name(), reloc->addend());
                    }

                    llvm::Constant* cexpr =
                        llvm::ConstantExpr::getAdd(
                            shadowImage->getSection(sec->name()),
                            _ctx->c.u32(addr));
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
            uint32_t val = *(uint32_t*)&bytes[addr];
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
