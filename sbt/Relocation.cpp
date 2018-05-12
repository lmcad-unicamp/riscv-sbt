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

SBTRelocation::SBTRelocation(
    Context* ctx,
    ConstRelocIter ri,
    ConstRelocIter re,
    ConstSectionPtr section)
    :
    _ctx(ctx),
    _ri(ri),
    _re(re),
    _section(section),
    _cur(_ri == _re? nullptr : *_ri),
    _curP(nullptr)
{
}


ConstRelocationPtr SBTRelocation::getReloc(uint64_t addr)
{
    if (!_cur && !_curP)
        return nullptr;

    auto next = [&]() {
        xassert(_cur == *_ri);
        ++_ri;
        if (_ri != _re)
            _cur = *_ri;
        else
            _cur = nullptr;
    };

    auto nextP = [&]() {
        xassert(_curP == _proxyRelocs.front());
        _proxyRelocs.pop();
        if (!_proxyRelocs.empty())
            _curP = _proxyRelocs.front();
        else
            _curP = nullptr;
    };

    // check if we skipped some addresses and now need to
    // advance the iterator
    while (_cur && addr > _cur->offset())
        next();
    while (_curP && addr > _curP->offset())
        nextP();

    ConstRelocationPtr ret = nullptr;
    if (_cur && _cur->offset() == addr) {
        ret = _cur;
        next();
        xassert(!_cur || _cur->offset() != addr);
    } else if (_curP && _curP->offset() == addr) {
        ret = _curP;
        nextP();
        xassert(!_curP || _curP->offset() != addr);
    }

    return ret;
}


void SBTRelocation::skipRelocation(uint64_t addr)
{
    // check if there is a relocation for current address
    getReloc(addr);
}


void SBTRelocation::addProxyReloc(ConstRelocationPtr reloc, bool pcrel)
{
    const LLVMRelocation* llrel = static_cast<const LLVMRelocation*>(&*reloc);
    ProxyRelocation* hi = new ProxyRelocation(*llrel);
    ProxyRelocation* lo = new ProxyRelocation(*llrel);

    if (pcrel) {
        hi->setType(Relocation::PROXY_PCREL_HI);
        lo->setType(Relocation::PROXY_PCREL_LO);
        lo->setHiPC(hi->offset());
    } else {
        hi->setType(Relocation::PROXY_HI);
        lo->setType(Relocation::PROXY_LO);
    }
    lo->setOffset(hi->offset() + Constants::INSTRUCTION_SIZE);

    _proxyRelocs.emplace(hi);
    _proxyRelocs.emplace(lo);

    if (!_curP)
        _curP = _proxyRelocs.front();
}


llvm::Expected<llvm::Constant*>
SBTRelocation::handleRelocation(uint64_t addr, llvm::raw_ostream* os)
{
    ConstRelocationPtr reloc = getReloc(addr);

    // no more relocations exist
    if (!reloc)
        return nullptr;

    DBGS << __METHOD_NAME__ << llvm::formatv("({0:X+8})\n", addr);
    DBGF("{0}", reloc->str());

    std::function<llvm::Constant*(llvm::Constant* addr)> relfn;

    switch (reloc->type()) {
        case Relocation::PROXY_HI:
        case llvm::ELF::R_RISCV_HI20:
            // hi20 = (symbol_address + 0x800) >> 12
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* c = llvm::ConstantExpr::getAdd(
                    addr, _ctx->c.u32(0x800));
                c = llvm::ConstantExpr::getLShr(c, _ctx->c.u32(12));
                return c;
            };
            break;

        case Relocation::PROXY_LO:
        case llvm::ELF::R_RISCV_LO12_I:
            // lo12 = symbol_address - hi20
            // hi20 = (symbol_address + 0x800) & 0xFFFFF000
            relfn = [this](llvm::Constant* addr) {
                llvm::Constant* hi20 = llvm::ConstantExpr::getAdd(
                    addr, _ctx->c.u32(0x800));
                hi20 = llvm::ConstantExpr::getAnd(
                    hi20, _ctx->c.u32(0xFFFFF000));

                llvm::Constant* c = llvm::ConstantExpr::getSub(addr, hi20);
                return c;
            };
            break;

        case Relocation::PROXY_PCREL_HI:
            // hi20 = (symbol_address - pc + 0x800) >> 12
            relfn = [this, addr](llvm::Constant* symaddr) {
                llvm::Constant* c = llvm::ConstantExpr::getSub(
                    symaddr, _ctx->c.u32(addr));
                c = llvm::ConstantExpr::getAdd(c, _ctx->c.u32(0x800));
                c = llvm::ConstantExpr::getLShr(c, _ctx->c.u32(12));
                return c;
            };
            break;

        case Relocation::PROXY_PCREL_LO:
            // lo12 = symbol_address - hipc - hi20
            // hi20 = (symbol_address - hipc + 0x800) & 0xFFFFF000
            relfn = [this, &reloc](llvm::Constant* symaddr) {
                const ProxyRelocation* pr =
                    static_cast<const ProxyRelocation*>(reloc.get());
                llvm::Constant* hipc = _ctx->c.u32(pr->hiPC());

                llvm::Constant* hi20 = llvm::ConstantExpr::getSub(
                    symaddr, hipc);
                hi20 = llvm::ConstantExpr::getAdd(hi20, _ctx->c.u32(0x800));
                hi20 = llvm::ConstantExpr::getAnd(
                        hi20, _ctx->c.u32(0xFFFFF000));

                llvm::Constant* c = llvm::ConstantExpr::getSub(
                    symaddr, hipc);
                c = llvm::ConstantExpr::getSub(c, hi20);
                return c;
            };
            break;

        case llvm::ELF::R_RISCV_CALL:
            addProxyReloc(reloc, PCREL);
            return handleRelocation(addr, os);

        case llvm::ELF::R_RISCV_LO12_S:
        case llvm::ELF::R_RISCV_PCREL_HI20:
        case llvm::ELF::R_RISCV_PCREL_LO12_I:
        case llvm::ELF::R_RISCV_PCREL_LO12_S:
        default:
            xunreachable("unknown relocation");
    }

    bool isLocalFunc = reloc->isLocalFunction();
    bool isExt = reloc->isExternal();
    reloc->validate();

    llvm::Constant* c = nullptr;

    // external symbol case: handle data or function
    if (isExt) {
        auto expAddr =_ctx->translator->import(reloc->symName());
        if (!expAddr)
            return expAddr.takeError();
        uint64_t saddr = expAddr.get().first;
        const std::string& xfunc = expAddr.get().second;

        // data
        if (saddr == SYM_TYPE_DATA) {
            DBGF("external data: saddr={0:X+8}", saddr);

            const Types& t = _ctx->t;
            c = _ctx->module->getOrInsertGlobal(reloc->symName(), t.i32);
            c = llvm::ConstantExpr::getPointerCast(c, t.i32);

        // handle external function
        } else {
            DBGF("external function: saddr={0:X+8}", saddr);
            c = _ctx->c.i32(saddr);
        }

        if (os)
            *os << xfunc;

    // internal function or label case
    // XXX we rely on symbol information to distinguish
    //     between function or label (see isFunction())
    } else if (isLocalFunc) {
        // get address
        uint64_t saddr;
        if (reloc->hasSym())
            saddr = reloc->symAddr();
        else
            saddr = reloc->addend();

        bool isFunc = Function::isFunction(_ctx, saddr);
        Function* f = _ctx->funcByAddr(saddr, !ASSERT_NOT_NULL);

        if (isFunc && !f) {
            // function not found: it should be ahead of current
            // translation address
            xassert(saddr > _ctx->addr);
            f = Function::getByAddr(_ctx, saddr);

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
        xassert(reloc->hasSec());
        uint64_t saddr;

        if (reloc->hasSym()) {
            DBGF("symbol reloc: symbol=\"{0}\", addend={1:X+8}, section=\"{2}\"",
                reloc->symName(), reloc->addend(), reloc->secName());

            saddr = reloc->symAddr() + reloc->addend();

            if (os)
                *os << reloc->symName();

        } else {
            DBGF("section reloc: section=\"{0}\", addend={1:X+8}",
                reloc->secName(), reloc->addend());

            saddr = reloc->addend();

            if (os)
                *os << llvm::formatv("{0}+{1:X+8}",
                    reloc->secName(), reloc->addend());
        }

        // TODO speed up section lookup for relocations
        c = llvm::ConstantExpr::getAdd(
            _ctx->shadowImage->getSection(reloc->secName()),
            _ctx->c.i32(saddr));
    }

    c = relfn(c);
    DBG(c->dump());
    return c;
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
                    uint64_t addr = reloc->addend();
                    xassert(reloc->hasSec());
                    if (reloc->hasSym()) {
                        addr += reloc->symAddr();
                        DBGF(
                            "relocating {0:X-8}: "
                            "symbol=\"{1}\", symaddr={2:X+8}, "
                            "section=\"{3}\", addend={4:X+8}, "
                            "addr={5:X-8}",
                            reloc->offset(),
                            reloc->symName(), reloc->symAddr(),
                            reloc->secName(), reloc->addend(),
                            addr);
                    } else {
                        DBGF(
                            "relocating {0:X-8}: "
                            "section=\"{1}\", addend={2:X+8}",
                            reloc->offset(),
                            reloc->secName(), reloc->addend());
                    }

                    llvm::Constant* cexpr =
                        llvm::ConstantExpr::getAdd(
                            shadowImage->getSection(reloc->secName()),
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
