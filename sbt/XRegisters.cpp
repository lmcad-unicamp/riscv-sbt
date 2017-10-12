#include "XRegisters.h"

#include "Builder.h"

#define GET_REGINFO_ENUM
#include <llvm/Target/RISCV/RISCVGenRegisterInfo.inc>

namespace sbt {

std::string XRegister::getName(unsigned reg, bool abi)
{
    if (!abi) {
        std::string s;
        llvm::raw_string_ostream ss(s);
        ss << "x" << reg;
        return ss.str();
    }

    switch (reg) {
        case ZERO: return "zero";
        case RA:     return "ra";
        case SP:     return "sp";
        case GP:     return "gp";
        case TP:     return "tp";
        case T0:     return "t0";
        case T1:     return "t1";
        case T2:     return "t2";
        case FP:     return "fp";
        case S1:     return "s1";
        case A0:     return "a0";
        case A1:     return "a1";
        case A2:     return "a2";
        case A3:     return "a3";
        case A4:     return "a4";
        case A5:     return "a5";
        case A6:     return "a6";
        case A7:     return "a7";
        case S2:     return "s2";
        case S3:     return "s3";
        case S4:     return "s4";
        case S5:     return "s5";
        case S6:     return "s6";
        case S7:     return "s7";
        case S8:     return "s8";
        case S9:     return "s9";
        case S10:    return "s10";
        case S11:    return "s11";
        case T3:     return "t3";
        case T4:     return "t4";
        case T5:     return "t5";
        case T6:     return "t6";
        default:     return "??";
    }
}

// MCInst register number to RISCV register number
unsigned XRegister::num(unsigned reg)
{
    namespace RISCV = llvm::RISCV;

    switch (reg) {
        case RISCV::X0:  return 0;
        case RISCV::X1:  return 1;
        case RISCV::X2:  return 2;
        case RISCV::X3:  return 3;
        case RISCV::X4:  return 4;
        case RISCV::X5:  return 5;
        case RISCV::X6:  return 6;
        case RISCV::X7:  return 7;
        case RISCV::X8:  return 8;
        case RISCV::X9:  return 9;
        case RISCV::X10: return 10;
        case RISCV::X11: return 11;
        case RISCV::X12: return 12;
        case RISCV::X13: return 13;
        case RISCV::X14: return 14;
        case RISCV::X15: return 15;
        case RISCV::X16: return 16;
        case RISCV::X17: return 17;
        case RISCV::X18: return 18;
        case RISCV::X19: return 19;
        case RISCV::X20: return 20;
        case RISCV::X21: return 21;
        case RISCV::X22: return 22;
        case RISCV::X23: return 23;
        case RISCV::X24: return 24;
        case RISCV::X25: return 25;
        case RISCV::X26: return 26;
        case RISCV::X27: return 27;
        case RISCV::X28: return 28;
        case RISCV::X29: return 29;
        case RISCV::X30: return 30;
        case RISCV::X31: return 31;
        default: return 0x1000;
    }
}


XRegister::XRegister(Context* ctx, unsigned num, uint32_t flags)
    :
    _num(num),
    _local(flags & LOCAL)
{
    if (_num == 0) {
        _x = nullptr;
        return;
    }

    const bool decl = flags & DECL;

    // get reg name and linkage type
    std::string s;
    llvm::raw_string_ostream ss(s);
    if (_local) {
        ss << "l" << getIRName() << _num;
        const std::string& name = ss.str();
        Builder* bld = ctx->bld;
        xassert(bld);
        _x = bld->_alloca(ctx->t.i32, nullptr, name);

    // global
    } else {
        ss << getIRName() << _num;
        llvm::GlobalVariable::LinkageTypes linkt =
            llvm::GlobalVariable::ExternalLinkage;
        const std::string& name = ss.str();
        _x = new llvm::GlobalVariable(*ctx->module, ctx->t.i32, !CONSTANT,
            linkt, decl? nullptr : ctx->c.ZERO, name);
    }
}


XRegisters::XRegisters(Context* ctx, uint32_t flags)
{
    // FIXME
    xassert(!(flags & DECL));

    _regs.reserve(NUM);
    for (size_t i = 0; i < NUM; i++)
        _regs.emplace_back(XRegister(ctx, i, flags));
}

}
