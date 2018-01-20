#ifndef SBT_XREGISTER_H
#define SBT_XREGISTER_H

#include "Register.h"


namespace sbt {

class XRegister : public Register
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


    /**
     * ctor.
     *
     * @param ctx
     * @param num register number
     * @param flags
     */
    XRegister(Context* ctx, unsigned num, uint32_t flags);

    // get RISC-V register number from llvm MCInst reg number
    static unsigned num(unsigned reg);

private:

    /**
     * Get register name.
     *
     * @param reg register number
     * @param abi abi or x?? name?
     */
    static std::string getName(unsigned reg, bool abi = true);
};


class XRegisters : public Registers
{
public:
    static const size_t NUM = 32;

    XRegisters(Context* ctx, uint32_t flags);
};

}

#endif
