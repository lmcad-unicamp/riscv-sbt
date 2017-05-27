#ifndef SBT_INSTRUCTION_H
#define SBT_INSTRUCTION_H

#include <llvm/IR/Value.h>
#include <llvm/MC/MCInst.h>
#include <llvm/Support/Error.h>

#include <cstdint>

namespace llvm {
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

  /**
   * ctor.
   *
   * @param ctx
   * @param addr instruction address
   * @param rawInst raw instruction bytes
   */
  Instruction(Context* ctx, uint64_t addr, uint32_t rawInst);

  // allow move only
  Instruction(Instruction&&) = default;
  Instruction& operator=(Instruction&&) = default;

  // dtor
  ~Instruction();

  // translate instruction
  llvm::Error translate();

private:
  Context* _ctx;
  Types* _t;
  Constants* _c;
  uint64_t _addr;
  uint32_t _rawInst;
  llvm::MCInst _inst;
  // debug output
  std::string _s;
  std::unique_ptr<llvm::raw_string_ostream> _ss;
  llvm::raw_ostream* _os;
  //
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

  // branch translation entrypoint
  llvm::Error translateBranch(BranchType bt);
  // call
  llvm::Error handleCall(uint64_t target);
  // indirect call
  llvm::Error handleICall(llvm::Value* target);
  // call to "external" function (for now, to libc functions)
  llvm::Error handleCallExt(llvm::Value* target);
  // jump to offset
  llvm::Error handleJumpToOffs(uint64_t target,
    llvm::Value* cond, unsigned linkReg);
  // indirect jump
  llvm::Error handleIJump(llvm::Value* target, unsigned linkReg);

  // syscall
  llvm::Error handleSyscall();

  // fence
  llvm::Error translateFence(bool fi);

  // CSR ops
  llvm::Error translateCSR(CSROp op, bool imm);

  // helpers

  // get RISCV destination register number
  unsigned getRD();
  // get RISCV register number (input is operand index)
  unsigned getRegNum(unsigned op);
  // get register value (input is operand index)
  llvm::Value* getReg(int op);
  // get immediate value
  llvm::Expected<llvm::Value*> getImm(int op);
  // get register or immediate
  llvm::Expected<llvm::Value*> getRegOrImm(int op);

  // add RV instr metadata and print it in debug mode
  // (no-op in release mode)
  void dbgprint();
};

}

#endif
