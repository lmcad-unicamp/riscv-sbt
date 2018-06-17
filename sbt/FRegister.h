#ifndef SBT_FREGISTER_H
#define SBT_FREGISTER_H

#include "Register.h"


namespace sbt {

class FRegister : public Register
{
public:
    enum Num : unsigned {
        FT0     = 0,
        FT1     = 1,
        FT2     = 2,
        FT3     = 3,
        FT4     = 4,
        FT5     = 5,
        FT6     = 6,
        FT7     = 7,
        FS0     = 8,
        FS1     = 9,
        FA0     = 10,
        FA1     = 11,
        FA2     = 12,
        FA3     = 13,
        FA4     = 14,
        FA5     = 15,
        FA6     = 16,
        FA7     = 17,
        FS2     = 18,
        FS3     = 19,
        FS4     = 20,
        FS5     = 21,
        FS6     = 22,
        FS7     = 23,
        FS8     = 24,
        FS9     = 25,
        FS10    = 26,
        FS11    = 27,
        FT8     = 28,
        FT9     = 29,
        FT10    = 30,
        FT11    = 31
    };

    /**
     * ctor.
     *
     * @param ctx
     * @param num register number
     * @param flags
     */
    FRegister(Context* ctx, unsigned num, uint32_t flags);

    // get RISC-V register number from llvm MCInst reg number
    static unsigned num(unsigned reg);

private:

    /**
     * Get register name.
     *
     * @param reg register number
     */
    static std::string getName(unsigned reg);
};


class FRegisters : public Registers
{
public:
    static const size_t NUM = 32;

    FRegisters(Context* ctx, uint32_t flags);
};

}

#endif
