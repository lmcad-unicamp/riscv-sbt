#ifndef SBT_REGISTER_H
#define SBT_REGISTER_H

#include "Context.h"
#include "Debug.h"

#include <string>
#include <vector>

namespace llvm {
class Value;
}

namespace sbt {

class Register
{
public:
    enum Flags : uint32_t {
        NONE  = 0,
        DECL  = 1,
        LOCAL = 2,
    };

    enum Type : uint32_t {
        T_INT,
        T_FLOAT
    };

    /**
     * ctor.
     *
     * @param ctx
     * @param num register number
     * @param name register name (for printing purposes)
     * @param irName register name (for IR generation)
     * @param type register type
     * @param flags
     */
    Register(
        Context* ctx,
        unsigned num,
        const std::string& name,
        const std::string& irName,
        Type type,
        uint32_t flags);

    // reg name
    const std::string& name() const
    {
        return _name;
    }

    llvm::Value* get()
    {
        _touched = true;
        return _r;
    }

    llvm::Value* getForRead()
    {
        _read = true;
        return _r;
    }

    bool hasRead() const
    {
        return _read;
    }

    llvm::Value* getForWrite()
    {
        _write = true;
        return _r;
    }

    bool hasWrite() const
    {
        return _write;
    }

    bool hasAccess() const
    {
        return _read || _write;
    }

    bool touched() const
    {
        return _touched || hasAccess();
    }

private:
    std::string _name;
protected:
    bool _local;
private:
    llvm::Value* _r;

    bool _read = false;
    bool _write = false;

    // this flag may remain false only while in main(),
    // because we don't load from global registers when
    // we enter it
    bool _touched = false;
};


class Registers
{
public:
    enum Flags : uint32_t {
        NONE  = 0,
        DECL  = 1,
        LOCAL = 2
    };

    Registers(std::vector<Register>&& regs) :
        _regs(std::move(regs))
    {}

    // get register by its number

    Register& getReg(size_t p)
    {
        xassert(p < _regs.size() && "register index is out of bounds");
        return _regs[p];
    }

private:
    std::vector<Register> _regs;
};


class CSR
{
public:
    enum Num : unsigned {
        FFLAGS      = 0x001,
        RDCYCLE     = 0xC00,
        RDTIME      = 0xC01,
        RDINSTRET   = 0xC02,
        RDCYCLEH    = 0xC80,
        RDTIMEH     = 0xC81,
        RDINSTRETH  = 0xC82
    };

    static const char* name(Num csr)
    {
#define CASE(csr) case csr: return #csr
        switch (csr) {
            CASE(FFLAGS);
            CASE(RDCYCLE);
            CASE(RDTIME);
            CASE(RDINSTRET);
            CASE(RDCYCLEH);
            CASE(RDTIMEH);
            CASE(RDINSTRETH);
        }
        xunreachable("invalid CSR");
#undef CASE
    }
};

}

#endif
