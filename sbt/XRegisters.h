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
    ZERO = 0,
    RA   = 1,    // Return address
    SP   = 2,    // Stack pointer
    GP   = 3,    // Global pointer
    TP   = 4,    // Thread pointer
    T0   = 5,    // Temporaries
    T1   = 6,
    T2   = 7,
    S0   = 8,    // Saved register
    FP   = 8,    // Frame pointer
    S1   = 9,
    A0   = 10,   // Function argument 0/return value 0
    A1   = 11,   // Function argument 1/return value 1
    A2   = 12,   // Function arguments
    A3   = 13,
    A4   = 14,
    A5   = 15,
    A6   = 16,
    A7   = 17,
    S2   = 18,
    S3   = 19,
    S4   = 20,
    S5   = 21,
    S6   = 22,
    S7   = 23,
    S8   = 24,
    S9   = 25,
    S10  = 26,
    S11  = 27,
    T3   = 28,
    T4   = 29,
    T5   = 30,
    T6   = 31
  };


  /**
   * ctor.
   * @param ctx
   * @param num register number
   * @param decl declare or define?
   */
  XRegister(Context* ctx, unsigned num, bool decl);

  // reg name
  const std::string& name() const
  {
    return _name;
  }

  // llvm variable
  llvm::GlobalVariable* var() const
  {
    return _x;
  }

  // get RISC-V register number from llvm MCInst reg number
  static unsigned num(unsigned reg);

private:
  unsigned _num;
  std::string _name = getName(_num);
  llvm::GlobalVariable* _x;


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
  XRegisters(Context* ctx, bool decl);

  // get register by its number
  const XRegister& operator[](size_t p) const
  {
    xassert(p < _regs.size() && "register index is out of bounds");
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
