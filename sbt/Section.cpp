#include "Section.h"

#include "Builder.h"
#include "Function.h"
#include "Instruction.h"
#include "Relocation.h"
#include "SBTError.h"

#include <llvm/Support/FormatVariadic.h>

namespace sbt {

llvm::Error SBTSection::translate()
{
    // skip non code sections
    if (!_section->isText())
        return llvm::Error::success();

    uint64_t addr = _section->address();
    uint64_t size = _section->size();

    // relocatable object?
    uint64_t elfOffset = addr;
    if (addr == 0)
        elfOffset = _section->getELFOffset();

    // print section info
    DBGS << llvm::formatv("section {0}: addr={1:X+4}, elfOffs={2:X+4}, "
        "size={3:X+4}\n",
        _section->name(), addr, elfOffset, size);

    // get relocations
    ConstObjectPtr obj = _section->object();
    const ConstRelocationPtrVec& relocs = obj->relocs();
    SBTRelocation reloc(_ctx, relocs.cbegin(), relocs.cend());
    _ctx->reloc = &reloc;

    Builder bld(_ctx);
    _ctx->bld = &bld;
    _ctx->sec = this;

    // get section bytes
    llvm::StringRef bytesStr;
    if (_section->contents(bytesStr)) {
        SBTError serr;
        serr << "failed to get section contents";
        return error(serr);
    }
    _bytes = llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(bytesStr.data()), bytesStr.size());

    const ConstSymbolPtrVec& symbols = _section->symbols();
    size_t n = symbols.size();
    Func func;

    enum State {
        INSIDE_FUNC,
        OUTSIDE_FUNC
    } state = OUTSIDE_FUNC;

    // for each symbol
    volatile uint64_t end;    // XXX gcc bug: need to make it volatile
    for (size_t i = 0; i < n; ++i) {
        ConstSymbolPtr sym = symbols[i];

        uint64_t symaddr = sym->address();
        if (i == n - 1)
            end = size;
        else
            end = symbols[i + 1]->address();

        // XXX for now, translate only instructions that appear after a
        // function or global symbol
        const llvm::StringRef symname = sym->name();
        if (sym->type() == llvm::object::SymbolRef::ST_Function ||
                sym->flags() & llvm::object::SymbolRef::SF_Global)
        {
            // function start
            if (state == OUTSIDE_FUNC) {
                func.name = symname;
                func.start = symaddr;
                state = INSIDE_FUNC;
            // function end
            } else {
                func.end = symaddr;
                // translate
                if (auto err = translate(func))
                    return err;
                // last symbol? get out
                if (i == n - 1)
                    state = OUTSIDE_FUNC;
                // start next functio
                else {
                    func.name = symname;
                    func.start = symaddr;
                    state = INSIDE_FUNC;
                }
            }

        // skip section bytes until a function like symbol is found
        } else if (state == OUTSIDE_FUNC) {
            DBGS << "skipping " << symname
                << llvm::formatv(": 0x{0:X+4}, 0x{1:X+4}", symaddr, uint64_t(end))
                << nl;
            continue;
        }
    }

    // finish last function
    if (state == INSIDE_FUNC) {
        func.end = end;
        if (auto err = translate(func))
            return err;
    }

    _ctx->bld = nullptr;
    _ctx->sec = nullptr;
    return llvm::Error::success();
}


llvm::Error SBTSection::translate(const Func& func)
{
    Function* f = new Function(_ctx, func.name, this, func.start, func.end);
    FunctionPtr fp(f);
    _ctx->addFunc(std::move(fp));
    return translate(f);
}


llvm::Error SBTSection::translate(Function* func)
{
    _ctx->f = func;
    updateNextFuncAddr(func->addr() + Instruction::SIZE);
    if (auto err = func->translate())
        return err;
    _ctx->f = nullptr;

    return llvm::Error::success();
}


void SBTSection::updateNextFuncAddr(uint64_t next)
{
    auto fmap = _ctx->_funcByAddr;
    auto nf = fmap->lower_bound(next);

    // no next function
    if (nf == fmap->end())
        return;

    _nextFuncAddr = nf->val->addr();
}

}
