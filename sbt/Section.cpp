#include "Section.h"

#include "Builder.h"
#include "Function.h"
#include "Instruction.h"
#include "Relocation.h"
#include "SBTError.h"

#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

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
    DBGF("section {0}: addr={1:X+8}, elfOffs={2:X+8}, "
        "size={3:X+8}",
        _section->name(), addr, elfOffset, size);

    DBGF("{0}", _section->str());

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
    if (_section->contents(bytesStr))
        return ERROR("failed to get section contents");
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
    uint64_t end;
    for (size_t i = 0; i < n; ++i) {
        ConstSymbolPtr sym = symbols[i];

        uint64_t symaddr = sym->address();
        if (i == n - 1)
            end = size;
        else
            end = symbols[i + 1]->address();

        // XXX function delimiters: global or function symbol
        std::string symname = sym->name();
        bool isValid = SBTSymbol::isFunction(sym) ||
            SBTSymbol::isGlobal(sym);

        DBGF("symname=[{0}], symaddr={1:X+8}, isValid={2}",
            symname, symaddr, isValid);

        if (isValid) {
            // function end
            if (state == INSIDE_FUNC) {
                func.end = symaddr;
                DBGF("function end: {0}@{1:X+8}", func.name, func.end);
                // translate
                if (auto err = translate(func))
                    return err;
                state = OUTSIDE_FUNC;
            }
            // function start
            if (state == OUTSIDE_FUNC) {
                func.name = symname;
                func.start = symaddr;
                state = INSIDE_FUNC;
                DBGF("function start: {0}@{1:X+8}", func.name, func.start);
            }
        } else
            DBGF("skipping non-function symbol @{0:X+8}: [{1}]", symaddr, symname);
    }

    // finish last function
    if (state == INSIDE_FUNC) {
        func.end = end;
        DBGF("last function end: {0}@{1:X+8}", func.name, func.end);
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
    updateNextFuncAddr(func->addr() + Constants::INSTRUCTION_SIZE);
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
