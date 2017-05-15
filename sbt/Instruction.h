#ifndef SBT_INSTRUCTION_H
#define SBT_INSTRUCTION_H

#include <llvm/Support/Error.h>

#include <cstdint>

namespace llvm {
class Instruction;
}

namespace sbt {

class Context;

class Instruction
{
public:
  static const std::size_t SIZE = 4;

  Instruction(Context* ctx, std::uint32_t rawInst) {}

  llvm::Error translate() { return llvm::Error::success(); }

  const llvm::Instruction* instr() const
  {
    return _instr;
  }

private:
  llvm::Instruction* _instr;

/*
  static const size_t InstrSize = 4;

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

  enum UIOp {
    AUIPC,
    LUI
  };

  enum IntType {
    S8,
    U8,
    S16,
    U16,
    U32
  };

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

  enum CSROp {
    RW,
    RS,
    RC
  };


  // get RD
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

  llvm::Error handleSyscall();

  // per instruction state
  llvm::Instruction* _first = nullptr;
  Imm _lastImm;

  // ALU op

  llvm::Error translateALUOp(
    const llvm::MCInst &Inst,
    ALUOp Op,
    uint32_t Flags,
    llvm::raw_string_ostream &SS);

  llvm::Error translateUI(
    const llvm::MCInst &Inst,
    UIOp UOP,
    llvm::raw_string_ostream &SS);

  // load/store

  llvm::Error translateLoad(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

  llvm::Error translateStore(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

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

  llvm::Error handleCall(uint64_t Target);
  llvm::Error handleICall(llvm::Value *Target);
  llvm::Error handleCallExt(llvm::Value *Target);

  // fence
  llvm::Error translateFence(const llvm::MCInst &Inst,
      bool FI,
      llvm::raw_string_ostream &SS);

  // CSR ops

  llvm::Error translateCSR(
      const llvm::MCInst &Inst,
      CSROp Op,
      bool Imm,
      llvm::raw_string_ostream &SS);
*/
};

/*
#if SBT_DEBUG
  // Add RV Inst metadata and print it in debug mode
  void dbgprint(llvm::raw_string_ostream &SS);
#else
  void dbgprint(const llvm::raw_string_ostream &SS) {}
#endif

  // host dependent ops
  llvm::Function* _getCycles = nullptr;
  llvm::Function* _getTime = nullptr;
  llvm::Function* _instRet = nullptr;
*/
}

#endif
