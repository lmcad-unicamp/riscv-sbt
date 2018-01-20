#include "FRegister.h"

#include "Builder.h"

#define GET_REGINFO_ENUM
#include <llvm/Target/RISCV/RISCVGenRegisterInfo.inc>

#include <llvm/Support/raw_ostream.h>

namespace sbt {

std::string FRegister::getName(unsigned reg)
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << "f" << reg;
    return ss.str();
}

// MCInst register number to RISCV register number
unsigned FRegister::num(unsigned reg)
{
    namespace RISCV = llvm::RISCV;

    switch (reg) {
        case RISCV::F0_64:  return 0;
        case RISCV::F1_64:  return 1;
        case RISCV::F2_64:  return 2;
        case RISCV::F3_64:  return 3;
        case RISCV::F4_64:  return 4;
        case RISCV::F5_64:  return 5;
        case RISCV::F6_64:  return 6;
        case RISCV::F7_64:  return 7;
        case RISCV::F8_64:  return 8;
        case RISCV::F9_64:  return 9;
        case RISCV::F10_64: return 10;
        case RISCV::F11_64: return 11;
        case RISCV::F12_64: return 12;
        case RISCV::F13_64: return 13;
        case RISCV::F14_64: return 14;
        case RISCV::F15_64: return 15;
        case RISCV::F16_64: return 16;
        case RISCV::F17_64: return 17;
        case RISCV::F18_64: return 18;
        case RISCV::F19_64: return 19;
        case RISCV::F20_64: return 20;
        case RISCV::F21_64: return 21;
        case RISCV::F22_64: return 22;
        case RISCV::F23_64: return 23;
        case RISCV::F24_64: return 24;
        case RISCV::F25_64: return 25;
        case RISCV::F26_64: return 26;
        case RISCV::F27_64: return 27;
        case RISCV::F28_64: return 28;
        case RISCV::F29_64: return 29;
        case RISCV::F30_64: return 30;
        case RISCV::F31_64: return 31;
        default:
            xunreachable("Invalid F register!");
    }
}


static const std::string FIRName = "rv_f";

static std::string getFRegIRName(unsigned num, bool local)
{
    // get reg name
    std::string s;
    llvm::raw_string_ostream ss(s);

    if (local)
        ss << "l" << FIRName << num;
    else
        ss << FIRName << num;

    ss.flush();
    return s;
}


FRegister::FRegister(Context* ctx, unsigned num, uint32_t flags)
    : Register(ctx, num,
            FRegister::getName(num),
            getFRegIRName(num, flags & LOCAL),
            Register::T_FLOAT, flags)
{
}


static std::vector<Register> createFRegisters(
    Context* ctx, uint32_t flags)
{
    // FIXME
    xassert(!(flags & DECL));

    std::vector<Register> regs;
    regs.reserve(FRegisters::NUM);

    for (size_t i = 0; i < FRegisters::NUM; i++)
        regs.emplace_back(FRegister(ctx, i, flags));

    return regs;
}

FRegisters::FRegisters(Context* ctx, uint32_t flags)
    : Registers(createFRegisters(ctx, flags))
{
}

}
