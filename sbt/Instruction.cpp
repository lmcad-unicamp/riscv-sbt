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
    *_os << llvm::formatv("{0:X-4}:\t", _addr);

    // disasm
    size_t size;
    if (auto err = _ctx->disasm->disasm(_addr, _rawInst, _inst, size)) {
        // handle invalid encoding
        llvm::Error err2 = llvm::handleErrors(std::move(err),
            [&](const InvalidInstructionEncoding& serr) {
                *_os << llvm::formatv("invalid({0:X+8})", _rawInst);
                // replace by nop
                // ADDI ZERO, ZERO, 0
                // imm[12]         rs1[5]  funct3[3]  rd[5]   opcode[7]
                // 0000 0000 0000  0000 0  000        0000 0  001 0011
                // _rawInst = 0x0000013;
                // if (auto err = _ctx->disasm->
                //        disasm(_addr, _rawInst, _inst, size))
                //    return err;

                _bld->nop();
                _ctx->reloc->skipRelocation(_addr);
                dbgprint();
                // return llvm::Error::success();
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
        // TODO group M instructions
        case RISCV::MUL:
            err = translateALUOp(MUL, AF_NONE);
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
            _ctx->syscall->call();
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
        case RISCV::FENCEI:
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
        default: {
            SBTError serr;
            serr << "unknown instruction opcode: " << _inst.getOpcode();
            return error(serr);
        }
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
    *_os << _ctx->x[nr].name() << ", ";
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

    *_os << _ctx->x[nr].name();
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
    xassert(false && "operand is neither a register nor immediate");
}


llvm::Expected<llvm::Value*> Instruction::getImm(int op)
{
    auto expV = _ctx->reloc->handleRelocation(_addr, _os);
    if (!expV)
        return expV.takeError();

    // case 1: symbol
    llvm::Value* v = expV.get();
    if (v)
        return v;

    // case 2: absolute immediate value
    int64_t imm = _inst.getOperand(op).getImm();
    v = _c->i32(imm);
    *_os << llvm::formatv("0x{0:X-4}", uint32_t(imm));
    return v;
}


llvm::Error Instruction::translateALUOp(ALUOp op, uint32_t flags)
{
    bool hasImm = flags & AF_IMM;
    bool isUnsigned = flags & AF_UNSIGNED;

    switch (op) {
        case ADD: *_os << "add";    break;
        case AND: *_os << "and";    break;
        case MUL: *_os << "mul";    break;
        case OR:    *_os << "or";     break;
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

        // TODO group M instructions
        case MUL:
            v = _bld->mul(o1, o2);
            break;

        case OR:
            v = _bld->_or(o1, o2);
            break;

        case SLL:
            v = _bld->sll(o1, o2);
            break;

        case SLT:
            if (isUnsigned)
                v = _bld->ult(o1, o2);
            else
                v = _bld->slt(o1, o2);
            v = _bld->zext(v);
            break;

        case SRA:
            v = _bld->sra(o1, o2);
            break;

        case SRL:
            v = _bld->srl(o1, o2);
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


llvm::Error Instruction::translateUI(UIOp op)
{
    switch (op) {
        case AUIPC: *_os << "auipc";    break;
        case LUI:     *_os << "lui";        break;
    }
    *_os << '\t';

    unsigned o = getRD();
    auto expImm = getImm(1);
    if (!expImm)
        return expImm.takeError();
    llvm::Value* imm = expImm.get();
    llvm::Value* v;

    if (_ctx->reloc->isSymbol(_addr))
        v = imm;
    else {
        // get upper immediate
        v = _bld->sll(imm, _c->i32(12));

        // add PC (current address)
        if (op == AUIPC)
            v = _bld->add(v, _c->i32(_addr));
    }

    _bld->store(v, o);

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
        llvm::SynchronizationScope::CrossThread);
    return llvm::Error::success();
}


llvm::Error Instruction::translateCSR(CSROp op, bool imm)
{
# define assert_nowr(cond) \
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
    *_os << llvm::formatv("0x{0:X-4} = ", csr);

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
# undef assert_nowr
}


llvm::Error Instruction::translateBranch(BranchType bt)
{
    // inst print
    switch (bt) {
        case JAL:     *_os << "jal";    break;
        case JALR:    *_os << "jalr"; break;
        case BEQ:     *_os << "beq";    break;
        case BNE:     *_os << "bne";    break;
        case BGE:     *_os << "bge";    break;
        case BGEU:    *_os << "bgeu"; break;
        case BLT:     *_os << "blt";    break;
        case BLTU:    *_os << "bltu"; break;
    }
    *_os << '\t';

    llvm::Error err = noError();

    // get operands
    unsigned o0n = getRegNum(0);
    llvm::Value* o0 = nullptr;
    unsigned o1n = 0;
    llvm::Value* o1 = nullptr;
    llvm::Value* o2 = nullptr;
    // jump register
    llvm::Value* jreg = nullptr;
    // jump immediate
    llvm::Value* jimm = nullptr;
    unsigned linkReg = 0;
    switch (bt) {
        case JAL:
            linkReg = o0n;
            if (!exp(getImm(1), o1, err))
                return err;
            jimm = o1;
            break;

        case JALR: {
            linkReg = o0n;
            // save os
            auto sos = _os;
            // set _os to nulls(), just to avoid producing incorrect debug output
            _os = &llvm::nulls();
            // get reg num
            o1n = getRegNum(1);
            // restore os
            _os = sos;
            // then get register value (this will produce correct debug output)
            o1 = getReg(1);
            if (!exp(getImm(2), o2, err))
                return err;
            jreg = o1;
            jimm = o2;
            break;
        }

        case BEQ:
        case BNE:
        case BGE:
        case BGEU:
        case BLT:
        case BLTU: {
            // save os
            auto sos = _os;
            // set _os to nulls(), just to avoid producing incorrect debug output
            _os = &llvm::nulls();
            // get reg
            o0 = getReg(0);
            // restore os
            _os = sos;

            o1 = getReg(1);
            if (!exp(getImm(2), o2, err))
                return err;
            jimm = o2;
            break;
        }
    }

    // jump immediate integer value
    int64_t jimmi = 0;
    // is jimm an immediate equal to zero?
    bool jimmIsZeroImm = false;
    bool isSymbol = _ctx->reloc->isSymbol(_addr);
    if (!isSymbol) {
        jimmi = llvm::cast<llvm::ConstantInt>(jimm)->getValue().getSExtValue();
        jimmIsZeroImm = jimmi == 0 && !isSymbol;
    }
    llvm::Value* v = nullptr;

    // return?
    // XXX using ABI info
    if (bt == JALR &&
            o0n == XRegister::ZERO &&
            o1n == XRegister::RA &&
            jimmIsZeroImm)
    {
        if (_ctx->inMain)
            v = _bld->ret(_bld->load(XRegister::A0));
        else
            v = _bld->retVoid();
        return llvm::Error::success();
    }

    // get target
    uint64_t target = 0;

    enum Action {
        JUMP_TO_SYMBOL,
        JUMP_TO_OFFS,
        IJUMP,
        CALL_SYMBOL,
        CALL_OFFS,
        CALL_EXT,
        ICALL
    };
    Action act;

    bool isCall = (bt == JAL || bt == JALR) &&
        linkReg != XRegister::ZERO;
    _ctx->brWasLast = !isCall;
    const SBTSymbol& sym = _ctx->reloc->last();

    // JALR
    //
    // TODO set target LSB to zero
    if (bt == JALR) {
        // no base reg
        if (o1n == XRegister::ZERO) {
            xassert(false && "unexpected JALR with base register equal to zero!");

        // no immediate
        } else if (jimmIsZeroImm) {
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

    // JAL to symbol
    } else if (bt == JAL && isSymbol) {
        if (isCall)
            act = CALL_SYMBOL;
        else
            act = JUMP_TO_SYMBOL;

    // JAL/branches to PC offsets
    //
    // add PC
    } else {
        target = jimmi + _addr;
        if (isCall)
            act = CALL_OFFS;
        else
            act = JUMP_TO_OFFS;
    }

    // process symbols
    if (act == CALL_SYMBOL || act == JUMP_TO_SYMBOL) {
        if (sym.isExternal()) {
            v = jimm;
            xassert(isCall && "jump to external module!");
            act = CALL_EXT;
        } else {
            xassert(sym.sec && sym.sec->isText() && "jump to non-text section!");
            target = sym.addr;
            if (isCall)
                act = CALL_OFFS;
            else
                act = JUMP_TO_OFFS;
        }
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

    // dispatch to appropriated handler
    switch (act) {
        case JUMP_TO_SYMBOL:
            xassert(false && "JUMP_TO_SYMBOL should become JUMP_TO_OFFS");
        case CALL_SYMBOL:
            xassert(false && "CALL_SYMBOL should become CALL_OFFS");

        case CALL_OFFS:
            err = handleCall(target, linkReg);
            break;

        case JUMP_TO_OFFS:
            err = handleJumpToOffs(target, cond, linkReg);
            break;

        case ICALL:
            err = handleICall(v);
            break;

        case CALL_EXT:
            err = handleCallExt(v);
            break;

        case IJUMP:
            err = handleIJump(v, linkReg);
            break;
    }

    return err;
}


llvm::Error Instruction::handleCall(uint64_t target, unsigned linkReg)
{
    DBGF("target={0:X+8}, linkReg={1}", target, linkReg);
    xassert(false && "TODO link register");

    // find function
    Function* f = Function::getByAddr(_ctx, target);
    _bld->call(f->func());
    return llvm::Error::success();
}


llvm::Error Instruction::handleICall(llvm::Value* target)
{
    // icaller expects the call target in register T1
    _bld->store(target, XRegister::T1);
    _bld->call(_ctx->translator->icaller().func());
    return llvm::Error::success();
}


llvm::Error Instruction::handleCallExt(llvm::Value* target)
{
    // at this point, 'target' will be the address of our external
    // call handler, set by Relocator previously
    llvm::FunctionType* ft = _t->voidFunc;
    llvm::Value* v = _bld->intToPtr(target, ft->getPointerTo());
    v = _bld->call(v);
    return llvm::Error::success();
}


template <typename Key, typename Value,
    typename Iter = typename sbt::Map<Key, Value>::Iter>
static Iter getTarget(sbt::Map<Key, Value>& map, const Key& target)
{
    Iter it = map.lower_bound(target);

    // target is the last entry
    if (it == map.end()) {
        xassert(!map.empty() && "empty map!");
        return --it;
    // entry matches target
    } else if (target == it->key)
        return it;
    // target is probably the previous one
    else if (it != map.begin())
        return --it;
    // target is the first one
    else
        return it;
}


static bool inBounds(
    uint64_t val,
    uint64_t begin,
    uint64_t end)
{
    return val >= begin && val < end;
}


llvm::Error Instruction::handleJumpToOffs(
    uint64_t target,
    llvm::Value* cond,
    unsigned linkReg)
{
    DBGF("target={0:X+8}, cond, linkReg={1:X+8}", target, linkReg);

    xassert(target != _addr && "unexpected jump to self instruction!");

    using BBMap = std::decay<decltype(((Function*)0)->bbmap())>::type;
    using BBIter = BBMap::Iter;

    const bool isCall = linkReg != XRegister::ZERO;
    const bool needNextBB = !isCall;
    const uint64_t nextInstrAddr = _addr + Instruction::SIZE;
    Function* func;
    BBMap* bbmap;
    BBMap* targetBBMap;
    BasicBlock* targetBB;

    // init vars
    func = _ctx->f;
    bbmap = &func->bbmap();
    targetBBMap = bbmap;

    // link
    if (!cond && isCall)
        _bld->store(_c->i32(nextInstrAddr), linkReg);

    // jump forward
    if (target > _addr) {
        DBGF("jump forward");

        // get basic block with start address >= target
        BBIter it = bbmap->lower_bound(target);

        // BB already exists
        if (target == it->key) {
            DBGF("target exists");
            targetBB = &*it->val;
        // need to create new BB
        } else {
            BasicBlock* beforeBB = it != bbmap->end()? &*it->val : nullptr;
            (*bbmap)(target, BasicBlockPtr(
                new BasicBlock(_ctx, target, func, beforeBB)));
            targetBB = &**(*bbmap)[target];
            func->updateNextBB(target);
        }

    // jump backward
    } else {
        DBGF("jump backward");

        auto targetInBounds = [](uint64_t target, BasicBlock* bb, const BBIter& it) {
            uint64_t begin = it->key;
            uint64_t end = it->key + bb->bb()->size() * Instruction::SIZE;
            return inBounds(target, begin, end);
        };

        BBIter it = getTarget(*bbmap, target);
        targetBB = &*it->val;
        bool inBound = targetInBounds(target, targetBB, it);

        // targetBB not found in current function: look in other functions
        if (!inBound) {
            auto fi = getTarget(*_ctx->_funcByAddr, target);
            Function* targetFunc = fi->val;
            targetBBMap = &targetFunc->bbmap();
            it = getTarget(*targetBBMap, target);
            targetBB = &*it->val;
            inBound = targetInBounds(target, targetBB, it);
            DBGF("target={0:X+8}, func={1}, targetFunc={2}, targetInBounds={3}",
                    target, func->name(), targetFunc->name(), inBound);
            xassert(false && "target basic block is in another function!");
        }

        // need to split targetBB?
        if (it->key != target) {
            (*targetBBMap)(target, targetBB->split(target));
            targetBB = &**(*targetBBMap)[target];
            // update insert point
            if (bbmap == targetBBMap)
                _bld->setInsertPoint(targetBB);
        }
    }

    if (needNextBB) {
        BasicBlock* beforeBB = nullptr;
        auto p = (*bbmap)[nextInstrAddr];
        // nextBB already exists?
        if (!p) {
            auto it = bbmap->lower_bound(nextInstrAddr);
            if (it != bbmap->end())
                beforeBB = &*it->val;

            (*bbmap)(nextInstrAddr, BasicBlockPtr(
                new BasicBlock(_ctx, nextInstrAddr, func, beforeBB)));
        }

        func->updateNextBB(nextInstrAddr);
    }

    // branch
    if (cond) {
        BasicBlock* next = &**(*bbmap)[nextInstrAddr];
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
    _bld->first()->setMetadata(mdname, n);
}

#else
void Instruction::dbgprint()
{}
#endif

}
