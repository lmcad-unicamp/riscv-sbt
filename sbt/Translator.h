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
  const std::string IREGNAME = "x";

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

  llvm::GlobalVariable *ShadowImage = nullptr;

  // Methods
  void buildRegisterFile();
  llvm::Error buildShadowImage();
  llvm::Error genSyscallHandler();

  llvm::Error handleSyscall(llvm::Instruction *&First);

  // Helpers

  void updateFirst(
    llvm::Value *V,
    llvm::Instruction *&First)
  {
    if (!First)
      First = llvm::dyn_cast<llvm::Instruction>(V);
  }

  // Load register
  llvm::LoadInst *load(
    unsigned Reg,
    llvm::Instruction *&First)
  {
    llvm::LoadInst *I = Builder->CreateLoad(X[Reg], X[Reg]->getName() + "_");
    updateFirst(I, First);
    return I;
  }

  // Store value to register
  llvm::StoreInst *store(
    llvm::Value *V,
    unsigned Reg,
    llvm::Instruction *&First)
  {
    llvm::StoreInst *I = Builder->CreateStore(V, X[Reg], !VOLATILE);
    updateFirst(I, First);
    return I;
  }

  // Add
  llvm::Value *add(
    llvm::Value *O1,
    llvm::Value *O2,
    llvm::Instruction *&First)
  {
    llvm::Value *V = Builder->CreateAdd(O1, O2);
    updateFirst(V, First);
    return V;
  }

  // Get RISC-V register number
  static unsigned RVReg(unsigned Reg);

  // Get RD
  unsigned getRD(
    const llvm::MCInst &Inst,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &OR = Inst.getOperand(0);
    unsigned NR = RVReg(OR.getReg());
    SS << IREGNAME << NR << ", ";
    return NR;
  }

  // Get register
  llvm::Value *getReg(
    const llvm::MCInst &Inst,
    int Op,
    llvm::Instruction *&First,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &OR = Inst.getOperand(Op);
    unsigned NR = RVReg(OR.getReg());
    llvm::Value *V;
    if (NR == 0)
      V = ZERO;
    else
      V = load(NR, First);

    SS << IREGNAME << NR;
    if (Op == 1)
       SS << ", ";
    return V;
  }

  // Get register or immediate
  llvm::Value *getRegOrImm(
    const llvm::MCInst &Inst,
    int Op,
    llvm::Instruction *&First,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &O = Inst.getOperand(Op);
    if (O.isReg())
      return getReg(Inst, Op, First, SS);
    else if (O.isImm())
      return getImm(Inst, Op, First, SS);
    else
      llvm_unreachable("Operand is neither a Reg nor Imm");
  }

  // Get immediate
  llvm::Value *getImm(
    const llvm::MCInst &Inst,
    int Op,
    llvm::Instruction *&First,
    llvm::raw_ostream &SS)
  {
    uint64_t Rel;
    if (handleRelocation(Rel, SS)) {
      llvm::Value *V =
        Builder->CreateGEP(ShadowImage, llvm::ConstantInt::get(I32, Rel));
      // V->dump();
      updateFirst(V, First);
      V = Builder->CreateCast(llvm::Instruction::CastOps::BitCast,
        V, llvm::Type::getInt32PtrTy(*Context));
      updateFirst(V, First);
      V = Builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
        V, I32);
      updateFirst(V, First);
      /*
      V = Builder->CreateLoad(V, "ShadowLoad");
      updateFirst(V, First);
       */
      return V;
    } else {
      int64_t Imm = Inst.getOperand(Op).getImm();
      llvm::Value *V = llvm::ConstantInt::get(I32, Imm);
      SS << Imm;
      return V;
    }
  }

  // Handle relocation.
  // Returns true if Rel was relocated.
  bool handleRelocation(
    uint64_t &Rel,
    llvm::raw_ostream &SS)
  {
    if (RI == RE)
      return false;

    auto CurReloc = *RI;
    if (CurReloc->offset() == CurAddr) {
      llvm::StringRef SymbolName = CurReloc->symbol()->name();
      llvm::StringRef RealSymbolName = SymbolName;
      // bool IsLO = false;
      if (SymbolName == ".L0 ") {
        auto LastReloc = *(RI - 1);
        // 12 lower bits
        // Rel = LastReloc->symbol()->address() & 0xFFF;
        Rel = LastReloc->symbol()->address();
        // IsLO = true;
        RealSymbolName = LastReloc->symbol()->name();
      } else {
        // 20 upper bits
        //Rel = CurReloc->symbol()->address() & 0xFFFFF000;
        Rel = CurReloc->symbol()->address();
      }

      /*
      if (IsLO)
        SS << "%lo(";
      else
        SS << "%hi(";
      SS << RealSymbolName << ") = " << Rel;
       */
      SS << RealSymbolName << " = " << Rel;

      ++RI;
      return true;
    }

    return false;
  }
};

} // sbt

#endif
