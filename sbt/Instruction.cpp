#include "Instruction.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Context.h"
#include "Disassembler.h"
#include "Relocation.h"
#include "SBTError.h"
#include "Section.h"
#include "Syscall.h"
#include "Translator.h"

#include <llvm/Support/FormatVariadic.h>

// LLVM internal instruction info
#define GET_INSTRINFO_ENUM
#include <llvm/Target/RISCV/RISCVGenInstrInfo.inc>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

Instruction::Instruction(Context* ctx, uint64_t addr, uint32_t rawInst)
    :
    _ctx(ctx),
    _t(&_ctx->t),
    _c(&_ctx->c),
    _addr(addr),
    _rawInst(rawInst),
#if SBT_DEBUG
    _ss(new llvm::raw_string_ostream(_s)),
    _os(&*_ss),
#else
    _os(&llvm::nulls()),
#endif
    _bld(_ctx->bld)
{
}


Instruction::~Instruction()
{
}


llvm::Error Instruction::translate()
{
    namespace RISCV = llvm::RISCV;

    // reset builder
    _bld->reset();

    // print address
    *_os << llvm::formatv("{0:X-8}:\t", _addr);

    // disasm
    size_t size;
    if (auto err = _ctx->disasm->disasm(_addr, _rawInst, _inst, size)) {
        // handle invalid encoding
        llvm::Error err2 = llvm::handleErrors(std::move(err),
            [&](const InvalidInstructionEncoding& serr) -> llvm::Error {
                *_os << llvm::formatv("invalid({0:X+8})", _rawInst);

                // handle all zero "instructions", that sometimes appear
                // because of alignment, replacing it by nop
                if (_rawInst == 0) {
                    _bld->nop();
                    _ctx->reloc->skipRelocation(_addr);
                    dbgprint();
                    return llvm::Error::success();
                } else
                    return error(InvalidInstructionEncoding(serr.msg()));
            });

        if (err2)
            return err2;
        else
            return llvm::Error::success();
    }

    llvm::Error err = noError();

    switch (_inst.getOpcode()) {
        // ALU ops
        case RISCV::ADD:
            err = translateALUOp(ADD, AF_NONE);
            break;
        case RISCV::ADDI:
            err = translateALUOp(ADD, AF_IMM);
            break;
        case RISCV::AND:
            err = translateALUOp(AND, AF_NONE);
            break;
        case RISCV::ANDI:
            err = translateALUOp(AND, AF_IMM);
            break;
        case RISCV::OR:
            err = translateALUOp(OR, AF_NONE);
            break;
        case RISCV::ORI:
            err = translateALUOp(OR, AF_IMM);
            break;
        case RISCV::SLL:
            err = translateALUOp(SLL, AF_NONE);
            break;
        case RISCV::SLLI:
            err = translateALUOp(SLL, AF_IMM);
            break;
        case RISCV::SLT:
            err = translateALUOp(SLT, AF_NONE);
            break;
        case RISCV::SLTU:
            err = translateALUOp(SLT, AF_UNSIGNED);
            break;
        case RISCV::SLTI:
            err = translateALUOp(SLT, AF_IMM);
            break;
        case RISCV::SLTIU:
            err = translateALUOp(SLT, AF_IMM | AF_UNSIGNED);
            break;
        case RISCV::SRA:
            err = translateALUOp(SRA, AF_NONE);
            break;
        case RISCV::SRAI:
            err = translateALUOp(SRA, AF_IMM);
            break;
        case RISCV::SRL:
            err = translateALUOp(SRL, AF_NONE);
            break;
        case RISCV::SRLI:
            err = translateALUOp(SRL, AF_IMM);
            break;
        case RISCV::SUB:
            err = translateALUOp(SUB, AF_NONE);
            break;
        case RISCV::XOR:
            err = translateALUOp(XOR, AF_NONE);
            break;
        case RISCV::XORI:
            err = translateALUOp(XOR, AF_IMM);
            break;

        // M instructions
        case RISCV::MUL:
            err = translateM(MUL);
            break;
        case RISCV::MULH:
            err = translateM(MULH);
            break;
        case RISCV::MULHSU:
            err = translateM(MULHSU);
            break;
        case RISCV::MULHU:
            err = translateM(MULHU);
            break;
        case RISCV::DIV:
            err = translateM(DIV);
            break;
        case RISCV::DIVU:
            err = translateM(DIVU);
            break;
        case RISCV::REM:
            err = translateM(REM);
            break;
        case RISCV::REMU:
            err = translateM(REMU);
            break;

        // UI (upper immediate)
        case RISCV::AUIPC:
            err = translateUI(AUIPC);
            break;
        case RISCV::LUI:
            err = translateUI(LUI);
            break;

        // branch
        case RISCV::BEQ:
            err = translateBranch(BEQ);
            break;
        case RISCV::BNE:
            err = translateBranch(BNE);
            break;
        case RISCV::BGE:
            err = translateBranch(BGE);
            break;
        case RISCV::BGEU:
            err = translateBranch(BGEU);
            break;
        case RISCV::BLT:
            err = translateBranch(BLT);
            break;
        case RISCV::BLTU:
            err = translateBranch(BLTU);
            break;
        // jump
        case RISCV::JAL:
            err = translateBranch(JAL);
            break;
        case RISCV::JALR:
            err = translateBranch(JALR);
            break;

        // ecall
        case RISCV::ECALL:
            *_os << "ecall";
            _ctx->translator->syscall().call();
            break;

        // ebreak
        case RISCV::EBREAK:
            *_os << "ebreak";
            _bld->nop();
            break;

        // load
        case RISCV::LB:
            err = translateLoad(S8);
            break;
        case RISCV::LBU:
            err = translateLoad(U8);
            break;
        case RISCV::LH:
            err = translateLoad(S16);
            break;
        case RISCV::LHU:
            err = translateLoad(U16);
            break;
        case RISCV::LW:
            err = translateLoad(U32);
            break;

        // store
        case RISCV::SB:
            err = translateStore(U8);
            break;
        case RISCV::SH:
            err = translateStore(U16);
            break;
        case RISCV::SW:
            err = translateStore(U32);
            break;

        // fence
        case RISCV::FENCE:
            err = translateFence(false);
            break;
        case RISCV::FENCE_I:
            err = translateFence(true);
            break;

        // CSR ops
        case RISCV::CSRRW:
            err = translateCSR(RW, false);
            break;
        case RISCV::CSRRWI:
            err = translateCSR(RW, true);
            break;
        case RISCV::CSRRS:
            err = translateCSR(RS, false);
            break;
        case RISCV::CSRRSI:
            err = translateCSR(RS, true);
            break;
        case RISCV::CSRRC:
            err = translateCSR(RC, false);
            break;
        case RISCV::CSRRCI:
            err = translateCSR(RC, true);
            break;

        // unknown
        default:
            return ERRORF("unknown instruction opcode: {0}", _inst.getOpcode());
    }

    if (err)
        return err;

    dbgprint();
    return llvm::Error::success();
}


unsigned Instruction::getRD()
{
    return getRegNum(0);
}


unsigned Instruction::getRegNum(unsigned op)
{
    const llvm::MCOperand& r = _inst.getOperand(op);
    unsigned nr = XRegister::num(r.getReg());
    *_os << _ctx->x->getReg(nr).name() << ", ";
    return nr;
}


llvm::Value* Instruction::getReg(int op)
{
    const llvm::MCOperand& mcop = _inst.getOperand(op);
    unsigned nr = XRegister::num(mcop.getReg());
    llvm::Value* v;
    if (nr == 0)
        v = _ctx->c.ZERO;
    else
        v = _bld->load(nr);

    *_os << _ctx->x->getReg(nr).name();
    if (op < 2)
         *_os << ", ";
    return v;
}


llvm::Expected<llvm::Value*> Instruction::getRegOrImm(int op)
{
    const llvm::MCOperand& o = _inst.getOperand(op);
    if (o.isReg())
        return getReg(op);
    else if (o.isImm())
        return getImm(op);
    xunreachable("operand is neither a register nor immediate");
}


llvm::Expected<llvm::Constant*> Instruction::getImm(int op)
{
    auto expC = _ctx->reloc->handleRelocation(_addr, _os);
    if (!expC)
        return expC.takeError();

    // case 1: symbol
    llvm::Constant* c = expC.get();
    if (c)
        return c;

    // case 2: absolute immediate value
    int64_t imm = _inst.getOperand(op).getImm();
    c = _c->i32(imm);
    *_os << llvm::formatv("0x{0:X-8}", uint32_t(imm));
    return c;
}


llvm::Error Instruction::translateALUOp(ALUOp op, uint32_t flags)
{
    bool hasImm = flags & AF_IMM;
    bool isUnsigned = flags & AF_UNSIGNED;

    switch (op) {
        case ADD: *_os << "add";    break;
        case AND: *_os << "and";    break;
        case OR:  *_os << "or";     break;
        case SLL: *_os << "sll";    break;
        case SLT: *_os << "slt";    break;
        case SRA: *_os << "sra";    break;
        case SRL: *_os << "srl";    break;
        case SUB: *_os << "sub";    break;
        case XOR: *_os << "xor";    break;
    }
    if (hasImm)
        *_os << "i";
    if (isUnsigned)
        *_os << "u";
    *_os << '\t';

    unsigned o = getRD();
    llvm::Value* o1 = getReg(1);
    llvm::Value* o2;
    if (hasImm) {
        auto expImm = getImm(2);
        if (!expImm)
            return expImm.takeError();
        o2 = expImm.get();
    } else
        o2 = getReg(2);

    llvm::Value* v;

    switch (op) {
        case ADD:
            v = _bld->add(o1, o2);
            break;

        case AND:
            v = _bld->_and(o1, o2);
            break;

        case OR:
            v = _bld->_or(o1, o2);
            break;

        case SLL:
            // Note: only the bottom 5 bits are valid on shifts
            v = _bld->_and(o2, _c->i32(0x1F));
            v = _bld->sll(o1, v);
            break;

        case SLT:
            if (isUnsigned)
                v = _bld->ult(o1, o2);
            else
                v = _bld->slt(o1, o2);
            v = _bld->zext(v);
            break;

        case SRA:
            v = _bld->_and(o2, _c->i32(0x1F));
            v = _bld->sra(o1, v);
            break;

        case SRL:
            v = _bld->_and(o2, _c->i32(0x1F));
            v = _bld->srl(o1, v);
            break;

        case SUB:
            v = _bld->sub(o1, o2);
            break;

        case XOR:
            v = _bld->_xor(o1, o2);
            break;
    }

    _bld->store(v, o);

    return llvm::Error::success();
}


llvm::Error Instruction::translateM(MOp op)
{
    switch (op) {
        case MUL:       *_os << "mul";      break;
        case MULH:      *_os << "mulh";     break;
        case MULHSU:    *_os << "mulhsu";   break;
        case MULHU:     *_os << "mulhu";    break;
        case DIV:       *_os << "div";      break;
        case DIVU:      *_os << "divu";     break;
        case REM:       *_os << "rem";      break;
        case REMU:      *_os << "remu";     break;
    }
    *_os << '\t';

    unsigned o = getRD();
    llvm::Value* o1 = getReg(1);
    llvm::Value* o2 = getReg(2);

    llvm::Value* v;
    switch (op) {
        case MUL:
            v = _bld->mul(o1, o2);
            break;

        case MULH:
            v = _bld->mulh(o1, o2);
            break;

        case MULHSU:
            v = _bld->mulhsu(o1, o2);
            break;

        case MULHU:
            v = _bld->mulhu(o1, o2);
            break;

        case DIV:
            v = _bld->div(o1, o2);
            break;

        case DIVU:
            v = _bld->divu(o1, o2);
            break;

        case REM:
            v = _bld->rem(o1, o2);
            break;

        case REMU:
            v = _bld->remu(o1, o2);
            break;
    }

    _bld->store(v, o);

    return llvm::Error::success();
}


llvm::Error Instruction::translateUI(UIOp op)
{
    switch (op) {
        case AUIPC: *_os << "auipc";    break;
        case LUI:   *_os << "lui";      break;
    }
    *_os << '\t';

    unsigned o = getRD();
    auto expImm = getImm(1);
    if (!expImm)
        return expImm.takeError();
    llvm::Constant* imm = expImm.get();

    // XXX review this
    /*
    if (_ctx->reloc->isSymbol(_addr))
        v = imm;
    else {
        // get upper immediate
        v = _bld->sll(imm, _c->i32(12));

        // add PC (current address)
        if (op == AUIPC)
            v = _bld->add(v, _c->i32(_addr));
    }
    */

    _bld->store(imm, o);

    return llvm::Error::success();
}


llvm::Error Instruction::translateLoad(IntType it)
{
    switch (it) {
        case S8:
            *_os << "lb";
            break;

        case U8:
            *_os << "lbu";
            break;

        case S16:
            *_os << "lh";
            break;

        case U16:
            *_os << "lhu";
            break;

        case U32:
            *_os << "lw";
            break;
    }
    *_os << '\t';

    unsigned o = getRD();
    llvm::Value* rs1 = getReg(1);
    auto expImm = getImm(2);
    if (!expImm)
        return expImm.takeError();
    llvm::Value* imm = expImm.get();

    llvm::Value* v = _bld->add(rs1, imm);

    llvm::Value* ptr;
    switch (it) {
        case S8:
        case U8:
            ptr = _bld->i32ToI8Ptr(v);
            break;

        case S16:
        case U16:
            ptr = _bld->i32ToI16Ptr(v);
            break;

        case U32:
            ptr = _bld->i32ToI32Ptr(v);
            break;
    }

    v = _bld->load(ptr);

    // to int32
    switch (it) {
        case S8:
        case S16:
            v = _bld->sext(v);
            break;

        case U8:
        case U16:
            v = _bld->zext(v);
            break;

        case U32:
            break;
    }
    _bld->store(v, o);

    return llvm::Error::success();
}


llvm::Error Instruction::translateStore(IntType it)
{
    switch (it) {
        case U8:
            *_os << "sb";
            break;

        case U16:
            *_os << "sh";
            break;

        case U32:
            *_os << "sw";
            break;

        default:
            xassert(false && "unknown store type!");
    }
    *_os << '\t';


    llvm::Value* rs2 = getReg(0);
    llvm::Value* rs1 = getReg(1);
    auto expImm = getImm(2);
    if (!expImm)
        return expImm.takeError();
    llvm::Value* imm = expImm.get();

    llvm::Value* v = _bld->add(rs1, imm);

    llvm::Value* ptr;
    switch (it) {
        case U8:
            ptr = _bld->i32ToI8Ptr(v);
            v = _bld->truncOrBitCastI8(rs2);
            break;

        case U16:
            ptr = _bld->i32ToI16Ptr(v);
            v = _bld->truncOrBitCastI16(rs2);
            break;

        case U32:
            ptr = _bld->i32ToI32Ptr(v);
            v = rs2;
            break;

        default:
            xassert(false && "unknown store type!");
    }

    _bld->store(v, ptr);

    return llvm::Error::success();
}


llvm::Error Instruction::translateFence(bool fi)
{
    if (fi) {
        *_os << "fence.i";
        _bld->nop();
        return llvm::Error::success();
    }

    *_os << "fence";

    _bld->fence(llvm::AtomicOrdering::AcquireRelease,
        llvm::SyncScope::System);
    return llvm::Error::success();
}


llvm::Error Instruction::translateCSR(CSROp op, bool imm)
{
#define assert_nowr(cond) \
        xassert(cond && "no CSR write support for base I instructions!")

    switch (op) {
        case RW:
            assert_nowr(false);
            break;

        case RS:
            *_os << "csrrs";
            break;

        case RC:
            *_os << "csrrc";
            break;
    }
    if (imm)
        *_os << "i";
    *_os << '\t';

    unsigned rd = getRegNum(0);
    uint64_t csr = _inst.getOperand(1).getImm();
    uint64_t src;
    if (imm) {
        src = _inst.getOperand(2).getImm();
        assert_nowr(src == 0);
    } else {
        src = getRegNum(2);
        assert_nowr(src == XRegister::ZERO);
    }
    *_os << llvm::formatv("0x{0:X-8} = ", csr);

    Translator* translator = _ctx->translator;
    translator->initCounters();
    llvm::Value* v = _c->ZERO;
    switch (csr) {
        case CSR::RDCYCLE:
            *_os << "RDCYCLE";
            v = _bld->call(_ctx->translator->getCycles().func());
            break;
        case CSR::RDCYCLEH:
            *_os << "RDCYCLEH";
            break;
        case CSR::RDTIME:
            *_os << "RDTIME";
            v = _bld->call(_ctx->translator->getTime().func());
            break;
        case CSR::RDTIMEH:
            *_os << "RDTIMEH";
            break;
        case CSR::RDINSTRET:
            *_os << "RDINSTRET";
            v = _bld->call(_ctx->translator->getInstRet().func());
            break;
        case CSR::RDINSTRETH:
            *_os << "RDINSTRETH";
            break;
        default:
            xassert(false && "not implemented!");
            break;
    }

    _bld->store(v, rd);

    return llvm::Error::success();
#undef assert_nowr
}


llvm::Error Instruction::translateBranch(BranchType bt)
{
    // inst print
    switch (bt) {
        case JAL:     *_os << "jal";    break;
        case JALR:    *_os << "jalr";   break;
        case BEQ:     *_os << "beq";    break;
        case BNE:     *_os << "bne";    break;
        case BGE:     *_os << "bge";    break;
        case BGEU:    *_os << "bgeu";   break;
        case BLT:     *_os << "blt";    break;
        case BLTU:    *_os << "bltu";   break;
    }
    *_os << '\t';

    llvm::Error err = noError();

    auto getRegNoLog = [this](int op) {
        // save os
        auto sos = _os;
        // set _os to nulls(), just to avoid producing incorrect debug output
        _os = &llvm::nulls();
        // get reg
        llvm::Value* v = getReg(op);
        // restore os
        _os = sos;
        return v;
    };

    auto getRegNumNoLog = [this](unsigned op) {
        // save os
        auto sos = _os;
        // set _os to nulls(), just to avoid producing incorrect debug output
        _os = &llvm::nulls();
        // get reg num
        unsigned n = getRegNum(op);
        // restore os
        _os = sos;
        return n;
    };

    // get operands
    unsigned linkReg;
    llvm::Value* o0 = nullptr;
    llvm::Value* o1 = nullptr;
    // jump register
    llvm::Value* jreg = nullptr;
    unsigned jregn = 0;
    // jump immediate
    llvm::Constant* jimm = nullptr;
    switch (bt) {
        case JAL:
            linkReg = getRegNum(0);
            if (!exp(getImm(1), jimm, err))
                return err;
            break;

        case JALR:
            linkReg = getRegNum(0);
            jregn = getRegNumNoLog(1);
            jreg = getReg(1);
            if (!exp(getImm(2), jimm, err))
                return err;
            break;

        case BEQ:
        case BNE:
        case BGE:
        case BGEU:
        case BLT:
        case BLTU:
            linkReg = XRegister::ZERO;
            o0 = getRegNoLog(0);
            o1 = getReg(1);
            if (!exp(getImm(2), jimm, err))
                return err;
            break;
    }

    llvm::Value* v = nullptr;

    // return?
    // XXX using ABI info
    if (bt == JALR &&
            linkReg == XRegister::ZERO &&
            jregn == XRegister::RA &&
            jimm->isZeroValue())
    {
        if (_ctx->inMain)
            v = _bld->ret(_bld->load(XRegister::A0));
        else {
            _ctx->f->storeRegisters();
            v = _bld->retVoid();
        }
        return llvm::Error::success();
    }

    // get target
    llvm::Constant* target = nullptr;

    enum Action {
        JUMP,
        IJUMP,
        CALL,
        ICALL
    };
    Action act;

    bool isCall = (bt == JAL || bt == JALR) &&
        linkReg != XRegister::ZERO;

    // JALR
    if (bt == JALR) {
        // no base reg
        if (jregn == XRegister::ZERO) {
            xunreachable("unexpected JALR with base register equal to zero!");

        // no immediate
        } else if (jimm->isZeroValue()) {
            v = jreg;
            if (isCall)
                act = ICALL;
            else
                act = IJUMP;

        // base + offset
        } else {
            v = _bld->add(jreg, jimm);
            if (isCall)
                act = ICALL;
            else
                act = IJUMP;
        }

    // JAL/branches to PC offsets
    //
    // add PC
    } else {
        target = llvm::ConstantExpr::getAdd(jimm, _ctx->c.i32(_addr));
        if (isCall)
            act = CALL;
        else
            act = JUMP;
    }

    // evaluate condition
    llvm::Value* cond = nullptr;
    switch (bt) {
        case BEQ:
            cond = _bld->eq(o0, o1);
            break;

        case BNE:
            cond = _bld->ne(o0, o1);
            break;

        case BGE:
            cond = _bld->sge(o0, o1);
            break;

        case BGEU:
            cond = _bld->uge(o0, o1);
            break;

        case BLT:
            cond = _bld->slt(o0, o1);
            break;

        case BLTU:
            cond = _bld->ult(o0, o1);
            break;

        case JAL:
        case JALR:
            break;
    }

    // target to integer
    auto targetInt = [target] {
        xassert(target);
        llvm::ConstantInt* ci = llvm::cast<llvm::ConstantInt>(target);
        return ci->getValue().getZExtValue();
    };

    // dispatch to appropriated handler
    switch (act) {
        case CALL:
            err = handleCall(targetInt(), linkReg);
            break;

        case JUMP:
            err = handleJump(targetInt(), cond, linkReg);
            break;

        case ICALL:
            err = handleICall(v, linkReg);
            break;

        case IJUMP:
            err = handleIJump(v, linkReg);
            break;
    }

    return err;
}


void Instruction::link(unsigned linkReg)
{
    if (linkReg == XRegister::ZERO)
        return;

    // link
    const uint64_t nextInstrAddr = _addr + Constants::INSTRUCTION_SIZE;
    _bld->store(_c->i32(nextInstrAddr), linkReg);
}


llvm::Error Instruction::handleCall(uint64_t target, unsigned linkReg)
{
    DBGF("target={0:X+8}, linkReg={1}", target, linkReg);

    // find function
    Function* f = Function::getByAddr(_ctx, target);

    link(linkReg);

    // call
    _ctx->f->storeRegisters();
    _bld->call(f->func());
    _ctx->f->loadRegisters();
    return llvm::Error::success();
}


llvm::Error Instruction::handleICall(llvm::Value* target, unsigned linkReg)
{
    DBGF("linkReg={1}", linkReg);
    const bool alwaysSync = true;

    // prepare
    Translator* translator = _ctx->translator;
    Function* f = _ctx->f;
    xassert(f);
    llvm::Function* llf = f->func();
    xassert(llf);
    const Function& ie = translator->isExternal();
    llvm::Function* llie = ie.func();
    xassert(llie);
    const Function& ic = translator->icaller();
    llvm::Function* llic = ic.func();
    xassert(llic);

    // link
    link(linkReg);

    // isExternal?
    llvm::Value* ext;
    BasicBlock* bbSaveRegs;
    BasicBlock* bbICall;


    // XXX FIXME for debugging only - uncomment this later
    //ext = _bld->call(llie, target);
    if (!alwaysSync) {
        ext = _c->i32(0);
        ext = _bld->eq(ext, _c->i32(1));
        bbSaveRegs = f->newUBB(_addr, "save_regs");
        bbICall = f->newUBB(_addr, "icall");
        _bld->condBr(ext, bbICall, bbSaveRegs);

        // save regs
        _bld->setInsertBlock(bbSaveRegs);
    }

    f->storeRegisters();

    if (!alwaysSync) {
        _bld->br(bbICall);
        _bld->setInsertBlock(bbICall);
    }

    // set args
    size_t n = llic->arg_size();
    std::vector<llvm::Value*> args;
    args.reserve(n);
    args.push_back(target);
    size_t i = 1;
    unsigned reg = XRegister::A0;
    for (; i < n; i++, reg++) {
        XRegister& x = f->getReg(reg);
        if (!x.touched())
            break;
        args.push_back(_bld->load(reg));
    }
    for (; i < n; i++)
        args.push_back(_ctx->c.ZERO);

    // dump args
    // for (auto arg : args)
    //    arg->dump();

    // call
    _bld->call(llic, args);

    // restore regs
    if (alwaysSync)
        f->loadRegisters();
    else if (!alwaysSync) {
        BasicBlock *bbRestoreRegs = f->newUBB(_addr, "restore_regs");
        BasicBlock *bbICallEnd = f->newUBB(_addr, "icall_end");
        _bld->condBr(ext, bbICallEnd, bbRestoreRegs);

        _bld->setInsertBlock(bbRestoreRegs);
        f->loadRegisters();
        _bld->br(bbICallEnd);

        _bld->setInsertBlock(bbICallEnd);
    }

    return llvm::Error::success();
}


static bool inBounds(
    uint64_t val,
    uint64_t begin,
    uint64_t end)
{
    return val >= begin && val < end;
}


llvm::Error Instruction::handleJump(
    uint64_t target,
    llvm::Value* cond,
    unsigned linkReg)
{
    DBGF("target={0:X+8}, cond, linkReg={1:X+8}", target, linkReg);

    xassert(target != _addr && "unexpected jump to self instruction!");

    const bool isCall = linkReg != XRegister::ZERO;
    const bool needNextBB = !isCall;
    const uint64_t nextInstrAddr = _addr + Constants::INSTRUCTION_SIZE;
    Function* func;
    BasicBlock* targetBB;

    // init vars
    func = _ctx->f;

    // jump forward
    if (target > _addr) {
        DBGF("jump forward");

        // get basic block with start address >= target
        BasicBlock* bb = func->lowerBoundBB(target);

        // BB already exists
        if (bb && target == bb->addr()) {
            DBGF("target exists");
            targetBB = bb;
        // need to create new BB
        } else {
            targetBB = func->newBB(target, bb);
            func->updateNextBB(target);
        }

    // jump backward
    } else {
        DBGF("jump backward");

        auto targetInBounds = [](uint64_t target, BasicBlock* bb) {
            uint64_t begin = bb->addr();
            uint64_t end = begin + bb->bb()->size() *
                Constants::INSTRUCTION_SIZE;
            return inBounds(target, begin, end);
        };

        targetBB = func->getBackBB(target);
        bool inBound = targetInBounds(target, targetBB);
        xassert(inBound && "target basic block not found!");

        // need to split targetBB?
        if (targetBB->addr() != target) {
            // if we're on the BB that's going to be splitted
            // then we need to switch to the lower part
            BasicBlock* curBB = _bld->getInsertBlock();
            bool switchToTargetBB = curBB == targetBB;

            targetBB = func->addBB(target, targetBB->split(target));

            // switch
            if (switchToTargetBB)
                _bld->setInsertBlock(targetBB);
        }
    }

    // link
    if (!cond && isCall)
        _bld->store(_c->i32(nextInstrAddr), linkReg);

    if (needNextBB) {
        BasicBlock* beforeBB = nullptr;
        BasicBlock* bb = func->findBB(nextInstrAddr);
        // nextBB already exists?
        if (!bb) {
            beforeBB = func->lowerBoundBB(nextInstrAddr);
            func->newBB(nextInstrAddr, beforeBB);
        }

        func->updateNextBB(nextInstrAddr);
    }

    // branch
    if (cond) {
        BasicBlock* next = func->findBB(nextInstrAddr);
        if (targetBB->addr() == next->addr())
            DBGF("WARNING: conditional branch to next instruction");
        _bld->condBr(cond, targetBB, next);
    } else
        _bld->br(targetBB);

    return llvm::Error::success();
}


llvm::Error Instruction::handleIJump(llvm::Value* target, unsigned linkReg)
{
    xassert(false && "indirect jump translation not implemented!");
}


#if SBT_DEBUG
// MD: metadata
// convert non-printable metadata chars
static std::string getMDName(const llvm::StringRef& s)
{
    std::string sss;
    llvm::raw_string_ostream ss(sss);
    ss << '_';
    for (char c : s) {
        switch (c) {
            case ' ':
            case ':':
            case ',':
            case '%':
            case '(':
            case ')':
            case '\t':
                ss << '_';
                break;

            case '=':
                ss << "eq";
                break;

            default:
                ss << c;
        }
    }
    return ss.str();
}


void Instruction::dbgprint()
{
    DBGS << _ss->str() << nl;
    llvm::MDNode* n = llvm::MDNode::get(*_ctx->ctx,
        llvm::MDString::get(*_ctx->ctx, "RISC-V Instruction"));
    std::string mdname = getMDName(_ss->str());
    // DBGF("mdname={0}, n={1:X+8}, first={2:X+8}", mdname, n, _bld->first());
    xassert(_bld->first());
    _bld->first()->setMetadata(mdname, n);
}

#else
void Instruction::dbgprint()
{}
#endif

}
