#include "Register.h"

#define GET_REGINFO_ENUM
#include <llvm/Target/RISCVMaster/RISCVMasterGenRegisterInfo.inc>

namespace sbt {

std::string regName(unsigned reg, bool abi)
{
  if (!abi) {
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << "x" << reg;
    return ss.str();
  }

  switch (reg) {
    case RV_ZERO: return "zero";
    case RV_RA:   return "ra";
    case RV_SP:   return "sp";
    case RV_GP:   return "gp";
    case RV_TP:   return "tp";
    case RV_T0:   return "t0";
    case RV_T1:   return "t1";
    case RV_T2:   return "t2";
    case RV_FP:   return "fp";
    case RV_S1:   return "s1";
    case RV_A0:   return "a0";
    case RV_A1:   return "a1";
    case RV_A2:   return "a2";
    case RV_A3:   return "a3";
    case RV_A4:   return "a4";
    case RV_A5:   return "a5";
    case RV_A6:   return "a6";
    case RV_A7:   return "a7";
    case RV_S2:   return "s2";
    case RV_S3:   return "s3";
    case RV_S4:   return "s4";
    case RV_S5:   return "s5";
    case RV_S6:   return "s6";
    case RV_S7:   return "s7";
    case RV_S8:   return "s8";
    case RV_S9:   return "s9";
    case RV_S10:  return "s10";
    case RV_S11:  return "s11";
    case RV_T3:   return "t3";
    case RV_T4:   return "t4";
    case RV_T5:   return "t5";
    case RV_T6:   return "t6";
    default:      return "??";
  }
}

// MCInst register number to RISCV register number
unsigned RVReg(unsigned Reg)
{
  namespace RISCV = llvm::RISCVMaster;

  switch (Reg) {
    case RISCV::X0_32:  return 0;
    case RISCV::X1_32:  return 1;
    case RISCV::X2_32:  return 2;
    case RISCV::X3_32:  return 3;
    case RISCV::X4_32:  return 4;
    case RISCV::X5_32:  return 5;
    case RISCV::X6_32:  return 6;
    case RISCV::X7_32:  return 7;
    case RISCV::X8_32:  return 8;
    case RISCV::X9_32:  return 9;
    case RISCV::X10_32: return 10;
    case RISCV::X11_32: return 11;
    case RISCV::X12_32: return 12;
    case RISCV::X13_32: return 13;
    case RISCV::X14_32: return 14;
    case RISCV::X15_32: return 15;
    case RISCV::X16_32: return 16;
    case RISCV::X17_32: return 17;
    case RISCV::X18_32: return 18;
    case RISCV::X19_32: return 19;
    case RISCV::X20_32: return 20;
    case RISCV::X21_32: return 21;
    case RISCV::X22_32: return 22;
    case RISCV::X23_32: return 23;
    case RISCV::X24_32: return 24;
    case RISCV::X25_32: return 25;
    case RISCV::X26_32: return 26;
    case RISCV::X27_32: return 27;
    case RISCV::X28_32: return 28;
    case RISCV::X29_32: return 29;
    case RISCV::X30_32: return 30;
    case RISCV::X31_32: return 31;
    default: return 0x1000;
  }
}

}
