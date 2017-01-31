#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Constants.h"
#include "Object.h"
#include "SBTError.h"

#include <llvm/MC/MCInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>

namespace llvm {
class LLVMContext;
class Function;
class FunctionType;
class IntegerType;
class GlobalVariable;
class MCInst;
class Module;
class Value;
}

namespace sbt {

class Translator
{
  static const bool HANDLE_RELOCATION = true;

  // RISC-V ABI
  static const unsigned RV_ZERO = 0;
  static const unsigned RV_RA = 1;    // Return address
  static const unsigned RV_SP = 2;    // Stack pointer
  static const unsigned RV_GP = 3;    // Global pointer
  static const unsigned RV_TP = 4;    // Thread pointer
  static const unsigned RV_T0 = 5;    // Temporaries
  static const unsigned RV_T1 = 6;
  static const unsigned RV_T2 = 7;
  static const unsigned RV_S0 = 8;    // Saved register
  static const unsigned RV_FP = 8;    // Frame pointer
  static const unsigned RV_S1 = 9;
  static const unsigned RV_A0 = 10;   // Function argument 0/return value 0
  static const unsigned RV_A1 = 11;   // Function argument 1/return value 1
  static const unsigned RV_A2 = 12;   // Function arguments
  static const unsigned RV_A3 = 13;
  static const unsigned RV_A4 = 14;
  static const unsigned RV_A5 = 15;
  static const unsigned RV_A6 = 16;
  static const unsigned RV_A7 = 17;
  static const unsigned RV_S2 = 18;
  static const unsigned RV_S3 = 19;
  static const unsigned RV_S4 = 20;
  static const unsigned RV_S5 = 21;
  static const unsigned RV_S6 = 22;
  static const unsigned RV_S7 = 23;
  static const unsigned RV_S8 = 24;
  static const unsigned RV_S9 = 25;
  static const unsigned RV_S10 = 26;
  static const unsigned RV_S11 = 27;
  static const unsigned RV_T3 = 28;
  static const unsigned RV_T4 = 29;
  static const unsigned RV_T5 = 30;
  static const unsigned RV_T6 = 31;

  static std::string regName(unsigned Reg, bool ABI = true)
  {
    if (!ABI) {
      std::string S;
      llvm::raw_string_ostream SS(S);
      SS << "x" << Reg;
      return SS.str();
    }

    switch (Reg) {
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

public:
  Translator(
    llvm::LLVMContext *Ctx,
    llvm::IRBuilder<> *Bldr,
    llvm::Module *Mod);

  Translator(Translator &&) = default;
  Translator(const Translator &) = delete;

  // translate one instruction
  llvm::Error translate(const llvm::MCInst &Inst);

  llvm::Error startModule();
  llvm::Error finishModule();

  llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);
  llvm::Error finishFunction();

  void setCurAddr(uint64_t Addr)
  {
    CurAddr = Addr;
  }

  void setCurObj(ConstObjectPtr Obj)
  {
    CurObj = Obj;
  }

  void setCurSection(ConstSectionPtr Section)
  {
    CurSection = Section;
  }

  typedef decltype(ConstRelocationPtrVec().cbegin()) ConstRelocIter;
  void setRelocIters(ConstRelocIter RI, ConstRelocIter RE)
  {
    this->RI = RI;
    this->RE = RE;
  }

private:
  // Constants
  llvm::IntegerType *I32;
  llvm::Value *ZERO;
  const std::string IR_XREGNAME = "x";

  // Data
  llvm::LLVMContext *Context;
  llvm::IRBuilder<> *Builder;
  llvm::Module *Module;
  llvm::GlobalVariable *X[32];
  llvm::FunctionType *FTRVSC;
  llvm::Function *FRVSC;

  uint64_t CurAddr;
  ConstObjectPtr CurObj = nullptr;
  ConstSectionPtr CurSection;
  ConstRelocIter RI;
  ConstRelocIter RE;
  llvm::Instruction *First = nullptr;

  llvm::GlobalVariable *ShadowImage = nullptr;

  // Methods
  void buildRegisterFile();
  llvm::Error buildShadowImage();
  llvm::Error genSyscallHandler();

  llvm::Error handleSyscall();

  // Helpers

  void updateFirst(llvm::Value *V)
  {
    if (!First)
      First = llvm::dyn_cast<llvm::Instruction>(V);
  }

  // Load register
  llvm::LoadInst *load(unsigned Reg)
  {
    llvm::LoadInst *I = Builder->CreateLoad(X[Reg], X[Reg]->getName() + "_");
    updateFirst(I);
    return I;
  }

  // Store value to register
  llvm::StoreInst *store(llvm::Value *V, unsigned Reg)
  {
    llvm::StoreInst *I = Builder->CreateStore(V, X[Reg], !VOLATILE);
    updateFirst(I);
    return I;
  }

  // Add
  llvm::Value *add(llvm::Value *O1, llvm::Value *O2)
  {
    llvm::Value *V = Builder->CreateAdd(O1, O2);
    updateFirst(V);
    return V;
  }

  // Get RISC-V register number
  static unsigned RVReg(unsigned Reg);

  // Get RD
  unsigned getRD(const llvm::MCInst &Inst, llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &OR = Inst.getOperand(0);
    unsigned NR = RVReg(OR.getReg());
    SS << regName(NR) << ", ";
    return NR;
  }

  // Get register
  llvm::Value *getReg(
    const llvm::MCInst &Inst,
    int Op,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &OR = Inst.getOperand(Op);
    unsigned NR = RVReg(OR.getReg());
    llvm::Value *V;
    if (NR == 0)
      V = ZERO;
    else
      V = load(NR);

    SS << regName(NR);
    if (Op == 1)
       SS << ", ";
    return V;
  }

  // Get register or immediate
  llvm::Value *getRegOrImm(
    const llvm::MCInst &Inst,
    int Op,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &O = Inst.getOperand(Op);
    if (O.isReg())
      return getReg(Inst, Op, SS);
    else if (O.isImm())
      return getImm(Inst, Op, SS);
    else
      llvm_unreachable("Operand is neither a Reg nor Imm");
  }

  // Get immediate
  llvm::Value *getImm(
    const llvm::MCInst &Inst,
    int Op,
    llvm::raw_ostream &SS)
  {
    if (llvm::Value *V = handleRelocation(SS))
      return V;

    int64_t Imm = Inst.getOperand(Op).getImm();
    llvm::Value *V = llvm::ConstantInt::get(I32, Imm);
    SS << Imm;
    return V;
  }

  bool isRelocation(const llvm::Value *V) const
  {
    return !llvm::isa<llvm::ConstantInt>(V);
  }

  // Handle relocation.
  // Returns true if Rel was relocated.
  llvm::Value *handleRelocation(llvm::raw_ostream &SS)
  {
    // No more relocations exist
    if (RI == RE)
      return nullptr;

    // Check if there is a relocation for current address
    auto CurReloc = *RI;
    if (CurReloc->offset() != CurAddr)
      return nullptr;

    llvm::StringRef SymbolName = CurReloc->symbol()->name();
    llvm::StringRef RealSymbolName = SymbolName;
    bool IsLO = false;
    uint64_t Rel;
    uint64_t Mask;
    // %lo relocation
    if (SymbolName == ".L0 ") {
      IsLO = true;
      Mask = 0xFFF;
      // Real symbol info is at previous relocation
      auto LastReloc = *(RI - 1);
      Rel = LastReloc->symbol()->address();
      RealSymbolName = LastReloc->symbol()->name();
    // %hi relocation
    } else {
      Mask = 0xFFFFF000;
      Rel = CurReloc->symbol()->address();
    }
    // Increment relocation iterator
    ++RI;

    // Write relocation string
    if (IsLO)
      SS << "%lo(";
    else
      SS << "%hi(";
    SS << RealSymbolName << ") = " << Rel;

    // Now add the relocation offset to ShadowImage to get the final address

    // Get char * to memory
    llvm::Value *V =
      Builder->CreateGEP(ShadowImage, llvm::ConstantInt::get(I32, Rel));
    updateFirst(V);
    // Cast to int32_t *
    V = Builder->CreateCast(llvm::Instruction::CastOps::BitCast,
      V, llvm::Type::getInt32PtrTy(*Context));
    updateFirst(V);
    // Cast to int32_t
    V = Builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
      V, I32);
    updateFirst(V);

    // Finally, get only the upper or lower part of the result
    V = Builder->CreateAnd(V, llvm::ConstantInt::get(I32, Mask));
    updateFirst(V);

    return V;
  }

#if SBT_DEBUG
  // Add RV Inst metadata and print it in debug mode
  void dbgprint(llvm::raw_string_ostream &SS);
#else
  void dbgprint(const llvm::raw_string_ostream &SS) {}
#endif
};

} // sbt

#endif
