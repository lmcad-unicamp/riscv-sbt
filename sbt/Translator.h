#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Constants.h"
#include "Map.h"
#include "Object.h"
#include "SBTError.h"

#include <llvm/MC/MCInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ELF.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FormatVariadic.h>

namespace llvm {
class LLVMContext;
class Function;
class FunctionType;
class IntegerType;
class GlobalVariable;
class MCInst;
class Module;
class Target;
class Value;
}

namespace sbt {

class Translator
{
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

  // CSRs
  static const unsigned RDCYCLE = 0xC00;
  static const unsigned RDCYCLEH = 0xC80;
  static const unsigned RDTIME = 0xC01;
  static const unsigned RDTIMEH = 0xC81;
  static const unsigned RDINSTRET = 0xC02;
  static const unsigned RDINSTRETH = 0xC82;

public:
  Translator(
    llvm::LLVMContext *Ctx,
    llvm::IRBuilder<> *Bldr,
    llvm::Module *Mod);

  Translator(Translator &&) = default;
  Translator(const Translator &) = delete;

  llvm::Error genSCHandler();

  // translate one instruction
  llvm::Error translate(const llvm::MCInst &Inst);

  llvm::Error startModule();
  llvm::Error finishModule();

  llvm::Expected<llvm::Function *> createFunction(llvm::StringRef Name);
  llvm::Error startMain(llvm::StringRef Name, uint64_t Addr);
  llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);
  llvm::Error finishFunction();

  llvm::Error startup();

  llvm::Expected<uint64_t> import(llvm::StringRef Func);

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

  void setTarget(const llvm::Target *T)
  {
    TheTarget = T;
  }

  typedef decltype(ConstRelocationPtrVec().cbegin()) ConstRelocIter;
  void setRelocIters(ConstRelocIter RI, ConstRelocIter RE)
  {
    this->RI = RI;
    this->RE = RE;
    this->RLast = RI;
  }

private:
  // Constants
  llvm::IntegerType *I32;
  llvm::IntegerType *I16;
  llvm::IntegerType *I8;
  llvm::Type *I32Ptr;
  llvm::Type *I16Ptr;
  llvm::Type *I8Ptr;
  llvm::Value *ZERO;
  llvm::Type *Void;
  llvm::FunctionType *VoidFun;
  const std::string IR_XREGNAME = "x";

  // Data
  llvm::LLVMContext *Context;
  llvm::IRBuilder<> *Builder;
  llvm::Module *Module;
  llvm::Value *X[32];
  llvm::FunctionType *FTRVSC;
  llvm::Function *FRVSC;
  std::unique_ptr<llvm::Module> LCModule;
  const llvm::Target *TheTarget = nullptr;

  uint64_t CurAddr;
  ConstObjectPtr CurObj = nullptr;
  ConstSectionPtr CurSection;
  ConstRelocIter RI;
  ConstRelocIter RE;
  ConstRelocIter RLast;
  llvm::Instruction *First = nullptr;
  llvm::Function *CurFunc = nullptr;

  llvm::GlobalVariable *ShadowImage = nullptr;
  llvm::GlobalVariable *Stack = nullptr;
  llvm::Value *StackEnd = nullptr;
  size_t StackSize = 4096;

  // Relocation Info
  struct SymbolReloc
  {
    bool IsValid = false;
    std::string Name;
    uint64_t Addr = 0;
    ConstSectionPtr Sec = nullptr;
    uint64_t Val = 0;

    bool isExternal() const
    {
      return IsValid && Addr == 0 && !Sec;
    }
  };

  struct LastImm
  {
    bool IsSym;
    SymbolReloc SymRel;
  } LastImm;

  SymbolReloc XSyms[32];

  ConstSectionPtr BSS = nullptr;
  size_t BSSSize = 0;

  ///

  bool InMain = false;

  Map<uint64_t, llvm::BasicBlock *> BBMap;
  Map<uint64_t, llvm::Instruction *> InstrMap;
  Map<llvm::Function *, uint64_t> FunMap;
  uint64_t NextBB = 0;
  bool BrWasLast = false;

  // icaller
  std::vector<llvm::Function *> FunTable;
  llvm::Function *ICaller = nullptr;
  uint64_t ExtFunAddr = 0;


  // Methods
  llvm::Error declOrBuildRegisterFile(bool decl);

  llvm::Error declRegisterFile()
  {
    return declOrBuildRegisterFile(true);
  }

  llvm::Error buildRegisterFile()
  {
    return declOrBuildRegisterFile(false);
  }

  llvm::Error buildShadowImage();
  void declSyscallHandler();
  llvm::Error genSyscallHandler();
  llvm::Error buildStack();

  llvm::Error handleSyscall();

  llvm::Error genICaller();

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
    return getRegNum(0, Inst, SS);
  }

  // GetRegNum
  unsigned getRegNum(
    unsigned Num,
    const llvm::MCInst &Inst,
    llvm::raw_ostream &SS)
  {
    const llvm::MCOperand &R = Inst.getOperand(Num);
    unsigned NR = RVReg(R.getReg());
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
    if (Op < 2)
       SS << ", ";
    return V;
  }

  // Get register or immediate
  llvm::Expected<llvm::Value *> getRegOrImm(
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
  llvm::Expected<llvm::Value *> getImm(
    const llvm::MCInst &Inst,
    int Op,
    llvm::raw_ostream &SS)
  {
    auto ExpV = handleRelocation(SS);
    if (!ExpV)
      return ExpV.takeError();
    llvm::Value *V = ExpV.get();
    if (V)
      return V;

    int64_t Imm = Inst.getOperand(Op).getImm();
    V = llvm::ConstantInt::get(I32, Imm);
    SS << llvm::formatv("0x{0:X-4}", uint32_t(Imm));
    return V;
  }

  llvm::Value *i8PtrToI32(llvm::Value *V8)
  {
    // Cast to int32_t *
    llvm::Value *V =
      Builder->CreateCast(llvm::Instruction::CastOps::BitCast,
        V8, llvm::Type::getInt32PtrTy(*Context));
    updateFirst(V);
    // Cast to int32_t
    V = Builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
      V, I32);
    updateFirst(V);
    return V;
  }

  llvm::Expected<llvm::Value *> handleRelocation(llvm::raw_ostream &SS);

  static std::string getBBName(uint64_t Addr)
  {
    std::string BBName = "bb";
    llvm::raw_string_ostream SS(BBName);
    SS << llvm::Twine::utohexstr(Addr);
    SS.flush();
    return BBName;
  }

  enum ALUOp {
    ADD,
    AND,
    MUL,
    OR,
    SLL,
    SLT,
    SRA,
    SRL,
    SUB,
    XOR
  };

  enum ALUOpFlags {
    AF_NONE = 0,
    AF_IMM = 1,
    AF_UNSIGNED = 2
  };

  llvm::Error translateALUOp(
    const llvm::MCInst &Inst,
    ALUOp Op,
    uint32_t Flags,
    llvm::raw_string_ostream &SS);

  enum UIOp {
    AUIPC,
    LUI
  };

  llvm::Error translateUI(
    const llvm::MCInst &Inst,
    UIOp UOP,
    llvm::raw_string_ostream &SS);

  enum IntType {
    S8,
    U8,
    S16,
    U16,
    U32
  };

  llvm::Error translateLoad(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

  llvm::Error translateStore(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

  enum BranchType {
    JAL,
    JALR,
    BEQ,
    BNE,
    BGE,
    BGEU,
    BLT,
    BLTU
  };

  // branch/jump/call handlers

  llvm::Error translateBranch(
    const llvm::MCInst &Inst,
    BranchType BT,
    llvm::raw_string_ostream &SS);

  llvm::Error handleJumpToOffs(
    uint64_t Target,
    llvm::Value *Cond,
    unsigned LinkReg);

  llvm::Error handleIJump(
    llvm::Value *Target,
    unsigned LinkReg);

  llvm::BasicBlock *splitBB(
    llvm::BasicBlock *BB,
    uint64_t Addr);

  llvm::Error handleCall(uint64_t Target);
  llvm::Error handleICall(llvm::Value *Target);
  llvm::Error handleCallExt(llvm::Value *Target);

  void updateNextBB(uint64_t Addr)
  {
    if (NextBB <= CurAddr || Addr < NextBB)
      NextBB = Addr;
  }

  // fence
  llvm::Error translateFence(const llvm::MCInst &Inst,
      bool FI,
      llvm::raw_string_ostream &SS);

  enum CSROp {
    RW,
    RS,
    RC
  };
  llvm::Error translateCSR(
      const llvm::MCInst &Inst,
      CSROp Op,
      bool Imm,
      llvm::raw_string_ostream &SS);

#if SBT_DEBUG
  // Add RV Inst metadata and print it in debug mode
  void dbgprint(llvm::raw_string_ostream &SS);
#else
  void dbgprint(const llvm::raw_string_ostream &SS) {}
#endif
};

} // sbt

#endif
