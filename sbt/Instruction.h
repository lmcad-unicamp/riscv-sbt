#ifndef SBT_INSTRUCTION_H
#define SBT_INSTRUCTION_H

#include <llvm/MC/MCInst.h>
#include <llvm/Support/Error.h>

#include <cstdint>

namespace llvm {
class Instruction;
class MCInst;
}

namespace sbt {

class Builder;
class Context;

class Instruction
{
public:
  static const std::size_t SIZE = 4;

  Instruction(Context* ctx, uint64_t addr, uint32_t rawInst);

  Instruction(Instruction&&) = default;
  Instruction& operator=(Instruction&&) = default;

  ~Instruction();

  llvm::Error translate();

  const llvm::Instruction* instr() const
  {
    return nullptr;
  }

private:
  Context* _ctx;
  uint64_t _addr;
  uint32_t _rawInst;
  llvm::MCInst _inst;
  std::string _s;
  std::unique_ptr<llvm::raw_string_ostream> _ss;
  llvm::raw_ostream* _os;
  std::unique_ptr<Builder> _builder;


  // enums

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


  // methods

  // ALU op
  llvm::Error translateALUOp(ALUOp op, uint32_t flags);

  llvm::Error translateUI(UIOp op);

  // load/store
  llvm::Error translateLoad(IntType it);

  llvm::Error translateStore(IntType it);

  // branch/jump/call handlers
  llvm::Error translateBranch(BranchType bt);

  // syscall
  llvm::Error handleSyscall();

  // fence
  llvm::Error translateFence(bool fi);

  // CSR ops
  llvm::Error translateCSR(CSROp op, bool imm);

/*

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


  // per instruction state
  llvm::Instruction* _first = nullptr;
  Imm _lastImm;


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

*/

  // add RV instr metadata and print it in debug mode
  void dbgprint();
};

/*
  // host dependent ops
  llvm::Function* _getCycles = nullptr;
  llvm::Function* _getTime = nullptr;
  llvm::Function* _instRet = nullptr;
*/
}

#endif
