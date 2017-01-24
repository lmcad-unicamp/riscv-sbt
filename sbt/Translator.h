#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

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

public:
  Translator(
    llvm::LLVMContext *Ctx,
    llvm::IRBuilder<> *Bldr,
    llvm::Module *Mod);

  ~Translator();

  // translate one instruction
  llvm::Error translate(const llvm::MCInst &Inst);

  llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);

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
  // constants
  llvm::IntegerType *I32;
  llvm::Value *ZERO;
  // data
  llvm::LLVMContext *Context;
  llvm::IRBuilder<> *Builder;
  llvm::Module *Module;
  bool FirstFunction = true;
  llvm::GlobalVariable *X[32];
  llvm::FunctionType *FTSyscall4;
  llvm::Function *FSyscall4;

  uint64_t CurAddr;
  ConstObjectPtr CurObj = nullptr;
  ConstSectionPtr CurSection;
  ConstRelocIter RI;
  ConstRelocIter RE;

  // methods
  llvm::Value *load(unsigned Reg);
  void store(llvm::Value *V, unsigned Reg);

  void buildRegisterFile();

  unsigned RVReg(unsigned Reg);

  unsigned getRD(const llvm::MCInst &Inst)
  {
      const llvm::MCOperand &OR = Inst.getOperand(0);
      unsigned NR = RVReg(OR.getReg());
      DBGS << "X" << NR << ", ";
      return NR;
  }

  llvm::Value *getReg(const llvm::MCInst &Inst, int Op)
  {
      const llvm::MCOperand &OR = Inst.getOperand(Op);
      unsigned NR = RVReg(OR.getReg());
      llvm::Value *V;
      if (NR == 0)
        V = ZERO;
      else
        V = load(NR);
      if (Op == 1)
        DBGS << "X" << NR << ", ";
      else
        DBGS << "X" << NR;
      return V;
  }

  llvm::Value *getRegOrImm(const llvm::MCInst &Inst, int Op)
  {
      const llvm::MCOperand &O = Inst.getOperand(Op);
      llvm::Value *V;
      if (O.isReg())
        return getReg(Inst, Op);
      else if (O.isImm()) {
        std::string RelocStr;
        uint64_t Rel;
        if (handleRelocation(Rel, &RelocStr)) {
          DBGS << RelocStr;
          return llvm::ConstantInt::get(I32, Rel);
        } else {
          int64_t Imm = O.getImm();
          V = llvm::ConstantInt::get(I32, Imm);
          DBGS << Imm;
          return V;
        }
      } else
        llvm_unreachable("Operand is neither a Reg nor Imm");
  }

  int64_t getImm(const llvm::MCInst &Inst, int Op,
      bool handleReloc = !HANDLE_RELOCATION)
  {
    const llvm::MCOperand &O = Inst.getOperand(Op);
    if (O.isImm()) {
      uint64_t Rel = 0;
      if (handleReloc && handleRelocation(Rel))
        return Rel;
      else
        return O.getImm();
    } else
      llvm_unreachable("Operand is not an Imm");
  }

  bool handleRelocation(uint64_t &Rel, std::string *Out = nullptr)
  {
    if (RI == RE)
      return false;

    auto CurReloc = *RI;
    if (CurReloc->offset() == CurAddr) {
      llvm::StringRef SymbolName = CurReloc->symbol()->name();
      llvm::StringRef RealSymbolName = SymbolName;
      bool IsLO = false;
      if (SymbolName == ".L0 ") {
        auto LastReloc = *(RI - 1);
        // 12 lower bits
        Rel = LastReloc->symbol()->address() & 0xFFF;
        IsLO = true;
        RealSymbolName = LastReloc->symbol()->name();
      } else {
        // 20 upper bits
        Rel = CurReloc->symbol()->address() & 0xFFFFF000;
      }

      if (Out) {
        llvm::raw_string_ostream SS(*Out);
        if (IsLO)
          SS << "%lo(";
        else
          SS << "%hi(";
        SS << RealSymbolName << ") = " << Rel;
      }

      ++RI;
      return true;
    }

    return false;
  }
};

} // sbt

#endif
