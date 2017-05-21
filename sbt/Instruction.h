#ifndef SBT_INSTRUCTION_H
#define SBT_INSTRUCTION_H

#include <llvm/IR/Value.h>
#include <llvm/MC/MCInst.h>
#include <llvm/Support/Error.h>

#include <cstdint>

namespace llvm {
class Instruction;
class MCInst;
}

namespace sbt {

class Builder;
class Constants;
class Context;
class Types;

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
  Types* _t;
  Constants* _c;
  uint64_t _addr;
  uint32_t _rawInst;
  llvm::MCInst _inst;
  std::string _s;
  std::unique_ptr<llvm::raw_string_ostream> _ss;
  llvm::raw_ostream* _os;
  Builder* _bld;


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


  // helpers

  unsigned getRD();
  unsigned getRegNum(unsigned num);
  llvm::Value* getReg(int num);
  llvm::Expected<llvm::Value*> getImm(int op);
  llvm::Expected<llvm::Value*> getRegOrImm(int op);


/*

  // per instruction state
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
