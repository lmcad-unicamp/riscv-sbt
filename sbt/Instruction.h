#ifndef SBT_INSTRUCTION_H
#define SBT_INSTRUCTION_H

#include <llvm/IR/Value.h>
#include <llvm/MC/MCInst.h>
#include <llvm/Support/Error.h>

#include <cstdint>

namespace llvm {
class LoadInst;
class MCInst;
class StoreInst;
}

namespace sbt {

class Builder;
class Constants;
class Context;
class Function;
class Types;

class Instruction
{
public:
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

    enum MOp {
        MUL,
        MULH,
        MULHSU,
        MULHU,
        DIV,
        DIVU,
        REM,
        REMU
    };

    // floating point enums

    enum FType {
        F_SINGLE,
        F_DOUBLE
    };

    enum FPUOp {
        F_ADD,
        F_DIV,
        F_EQ,
        F_LE,
        F_LT,
        F_MIN,
        F_MAX,
        F_MUL,
        F_SGNJ,
        F_SGNJN,
        F_SGNJX,
        F_SQRT,
        F_SUB
    };

    enum IType {
        F_W,
        F_WU
    };

    // methods

    // ALU op
    llvm::Error translateALUOp(ALUOp op, uint32_t flags);
    llvm::Error translateUI(UIOp op);

    // load/store
    llvm::Error translateLoad(IntType it);
    llvm::Error translateStore(IntType it);

    // branch/jump/call handlers

    void link(unsigned linkReg);

    // branch translation entrypoint
    llvm::Error translateBranch(BranchType bt);
    // call
    llvm::Error handleCall(uint64_t target, unsigned linkReg);
    // indirect call
    llvm::Error handleICall(llvm::Value* target, unsigned linkReg);
    // jump to offset
    llvm::Error handleJump(uint64_t target,
        llvm::Value* cond, unsigned linkReg);
    // indirect jump
    llvm::Error handleIJump(llvm::Value* target);

    // syscall
    llvm::Error handleSyscall();

    // fence
    llvm::Error translateFence(bool fi);

    // CSR ops
    llvm::Error translateCSR(CSROp op, bool imm);

    // Multiply/divide extension
    llvm::Error translateM(MOp op);

    llvm::Value* leaveFunction(llvm::Value* target);
    void enterFunction(llvm::Value* ext);

    // helpers

    // get RISCV destination register number
    unsigned getRD();
    // get RISCV register number (input is operand index)
    unsigned getRegNum(unsigned op);
    // get register value (input is operand index)
    llvm::Value* getReg(int op);
    // get immediate value
    llvm::Expected<llvm::Constant*> getImm(int op);
    // get register or immediate
    llvm::Expected<llvm::Value*> getRegOrImm(int op);

    // add RV instr metadata and print it in debug mode
    // (no-op in release mode)
    void dbgprint();

    Function* findFunction(llvm::Constant* c) const;


    // float

    // handle F/D extension
    llvm::Error translateF();

    // FP load/store
    llvm::Error translateFPLoad(FType ft);
    llvm::Error translateFPStore(FType ft);

    // FPU op
    llvm::Error translateFPUOp(FPUOp op, FType ft);

    // CVT
    // fp to int
    llvm::Error translateCVT(IType it, FType ft);
    // int to fp
    llvm::Error translateCVT(FType ft, IType it);
    // fp to fp
    llvm::Error translateCVT(FType ft, FType it);

    // fmv
    // fp to int
    llvm::Error translateFMV(IType it, FType ft);
    // int to fp
    llvm::Error translateFMV(FType ft, IType it);

    // float helpers

    llvm::LoadInst* fload(unsigned reg, FType ty);
    llvm::StoreInst* fstore(llvm::Value* v, unsigned reg, FType ty);
    unsigned getFRD();
    unsigned getFRegNum(unsigned op);
    llvm::Value* getFReg(int op, FType ty);
};

}

#endif
