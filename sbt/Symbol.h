#ifndef SBT_SYMBOL_H
#define SBT_SYMBOL_H

#include "Object.h"

#include <cstdint>
#include <string>

namespace sbt {

class SBTSymbol
{
public:
    uint64_t addr;
    uint64_t val;
    std::string name;
    ConstSectionPtr sec;
    uint64_t instrAddr;


    SBTSymbol(
        uint64_t addr,
        uint64_t val,
        const std::string& name,
        ConstSectionPtr sec,
        uint64_t instrAddr)
        :
        addr(addr),
        val(val),
        name(name),
        sec(sec),
        instrAddr(instrAddr)
    {}


    bool isExternal() const
    {
        return addr == 0 && !sec;
    }

    bool isFunction() const
    {
        return sec && sec->isText();
    }

    static bool isGlobal(ConstSymbolPtr sym)
    {
        return sym->flags() & llvm::object::SymbolRef::SF_Global;
    }

    static bool isFunction(ConstSymbolPtr sym)
    {
        return sym->type() == llvm::object::SymbolRef::ST_Function;
    }
};

}

#endif
