#ifndef SBT_FREGISTER_H
#define SBT_FREGISTER_H

#include "Register.h"


namespace sbt {

class FRegister : public Register
{
public:
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
