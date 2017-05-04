#ifndef SBT_REGISTERS_H
#define SBT_REGISTERS_H

#include <llvm/Support/raw_ostream.h>

#include <string>

namespace llvm {
class GlobalVariable;
}

namespace sbt {

class Register
{
public:

  Register() = delete;

  // RISC-V ABI
  enum RVRegister : unsigned {
    RV_ZERO = 0,
    RV_RA   = 1,    // Return address
    RV_SP   = 2,    // Stack pointer
    RV_GP   = 3,    // Global pointer
    RV_TP   = 4,    // Thread pointer
    RV_T0   = 5,    // Temporaries
    RV_T1   = 6,
    RV_T2   = 7,
    RV_S0   = 8,    // Saved register
    RV_FP   = 8,    // Frame pointer
    RV_S1   = 9,
    RV_A0   = 10,   // Function argument 0/return value 0
    RV_A1   = 11,   // Function argument 1/return value 1
    RV_A2   = 12,   // Function arguments
    RV_A3   = 13,
    RV_A4   = 14,
    RV_A5   = 15,
    RV_A6   = 16,
    RV_A7   = 17,
    RV_S2   = 18,
    RV_S3   = 19,
    RV_S4   = 20,
    RV_S5   = 21,
    RV_S6   = 22,
    RV_S7   = 23,
    RV_S8   = 24,
    RV_S9   = 25,
    RV_S10  = 26,
    RV_S11  = 27,
    RV_T3   = 28,
    RV_T4   = 29,
    RV_T5   = 30,
    RV_T6   = 31
  };

  enum CSR : unsigned {
    RDCYCLE     = 0xC00,
    RDTIME      = 0xC01,
    RDINSTRET   = 0xC02,
    RDCYCLEH    = 0xC80,
    RDTIMEH     = 0xC81,
    RDINSTRETH  = 0xC82
  };

  static std::string getXRegName(unsigned reg, bool abi = true);

  // get RISC-V register number
  static unsigned getXReg(unsigned reg);

  static std::string getXRegName()
  {
    return "rv_x";
  }

  static llvm::GlobalVariable* rvX[32];
};

}

#endif
