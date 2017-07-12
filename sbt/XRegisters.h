#ifndef SBT_XREGISTERS_H
#define SBT_XREGISTERS_H

#include "Context.h"

#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>

namespace llvm {
class GlobalVariable;
}

namespace sbt {

class XRegister
{
public:
    // RISC-V ABI
    enum Num : unsigned {
        ZERO    = 0,
        RA      = 1,    // return address
        SP      = 2,    // stack pointer
        GP      = 3,    // global pointer
        TP      = 4,    // thread pointer
        T0      = 5,    // temporaries
        T1      = 6,
        T2      = 7,
        S0      = 8,    // saved register
        FP      = 8,    // frame pointer
        S1      = 9,
        A0      = 10,   // function argument 0/return value 0
        A1      = 11,   // function argument 1/return value 1
        A2      = 12,   // function arguments
        A3      = 13,
        A4      = 14,
        A5      = 15,
        A6      = 16,
        A7      = 17,
        S2      = 18,
        S3      = 19,
        S4      = 20,
        S5      = 21,
        S6      = 22,
        S7      = 23,
        S8      = 24,
        S9      = 25,
        S10     = 26,
        S11     = 27,
        T3      = 28,
        T4      = 29,
        T5      = 30,
        T6      = 31
    };

    enum Flags : uint32_t {
        NONE  = 0,
        DECL  = 1,
        LOCAL = 2
    };

    /**
     * ctor.
     * @param ctx
     * @param num register number
     * @param flags
     */
    XRegister(Context* ctx, unsigned num, uint32_t flags);

    // reg name
    const std::string& name() const
    {
        return _name;
    }

    // get RISC-V register number from llvm MCInst reg number
    static unsigned num(unsigned reg);

    llvm::Value* get()
    {
        _touched = true;
        return _x;
    }

    llvm::Value* getForRead()
    {
        _read = true;
        return _x;
    }

    bool hasRead() const
    {
        return _read;
    }

    llvm::Value* getForWrite()
    {
        _write = true;
        return _x;
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
    unsigned _num;
    std::string _name = getName(_num);
    llvm::Value* _x;
    bool _local;
    bool _read = false;
    bool _write = false;

    // this flag may remain false only while in main(),
    // because we don't load from global registers when
    // we enter it
    bool _touched = false;


    // register name on generated llvm IR
    static std::string getIRName()
    {
        return "rv_x";
    }

    /**
     * Get register name.
     *
     * @param reg register number
     * @param abi abi or x?? name?
     */
    static std::string getName(unsigned reg, bool abi = true);
};


/**
 * Register file.
 */
class XRegisters
{
public:
    static const size_t NUM = 32;

    enum Flags : uint32_t {
        NONE  = 0,
        DECL  = 1,
        LOCAL = 2
    };

    XRegisters(Context* ctx, uint32_t flags);

    // get register by its number

    XRegister& getReg(size_t p)
    {
        xassert(p < NUM && "register index is out of bounds");
        return _regs[p];
    }

private:
    std::vector<XRegister> _regs;
};


class CSR
{
public:
    enum Num : unsigned {
        RDCYCLE     = 0xC00,
        RDTIME      = 0xC01,
        RDINSTRET   = 0xC02,
        RDCYCLEH    = 0xC80,
        RDTIMEH     = 0xC81,
        RDINSTRETH  = 0xC82
    };
};

}

#endif
