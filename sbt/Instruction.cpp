#include "Instruction.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Context.h"
#include "Disassembler.h"
#include "Relocation.h"
#include "SBTError.h"
#include "Syscall.h"
#include "Translator.h"

#include <llvm/Support/FormatVariadic.h>

// LLVM internal instruction info
#define GET_INSTRINFO_ENUM
#include <llvm/Target/RISCVMaster/RISCVMasterGenInstrInfo.inc>


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
  namespace RISCV = llvm::RISCVMaster;

  // disasm
  size_t size;
  if (auto err = _ctx->disasm->disasm(_addr, _rawInst, _inst, size))
    return err;

  // print address
  *_os << llvm::formatv("{0:X-4}:   ", _addr);

  _bld->reset();
  llvm::Error err = noError();

  switch (_inst.getOpcode()) {
    // ALU Ops
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

    // UI
    case RISCV::AUIPC:
      err = translateUI(AUIPC);
      break;
    case RISCV::LUI:
      err = translateUI(LUI);
      break;

    // Branch
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
    // Jump
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

    // Load
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

    // Store
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

    // system
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

    default: {
      SBTError serr;
      serr << "Unknown instruction opcode: " << _inst.getOpcode();
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


unsigned Instruction::getRegNum(unsigned num)
{
  const llvm::MCOperand& r = _inst.getOperand(num);
  unsigned nr = XRegister::num(r.getReg());
  *_os << _ctx->x[nr].name() << ", ";
  return nr;
}


llvm::Value* Instruction::getReg(int num)
{
  const llvm::MCOperand& op = _inst.getOperand(num);
  unsigned nr = XRegister::num(op.getReg());
  llvm::Value* v;
  if (nr == 0)
    v = _ctx->c.ZERO;
  else
    v = _bld->load(nr);

  *_os << _ctx->x[nr].name();
  if (num < 2)
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
  xassert(false && "Operand is neither a Reg nor Imm");
}


llvm::Expected<llvm::Value*> Instruction::getImm(int op)
{
  auto expV = _ctx->reloc->handleRelocation(_addr, _os);
  if (!expV)
    return expV.takeError();
  llvm::Value* v = expV.get();
  if (v)
    return v;

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
    case ADD: *_os << "add";  break;
    case AND: *_os << "and";  break;
    case MUL: *_os << "mul";  break;
    case OR:  *_os << "or";   break;
    case SLL: *_os << "sll";  break;
    case SLT: *_os << "slt";  break;
    case SRA: *_os << "sra";  break;
    case SRL: *_os << "srl";  break;
    case SUB: *_os << "sub";  break;
    case XOR: *_os << "xor";  break;
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
    auto ExpImm = getImm(2);
    if (!ExpImm)
      return ExpImm.takeError();
    o2 = ExpImm.get();
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
    case AUIPC: *_os << "auipc";  break;
    case LUI:   *_os << "lui";    break;
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

    // add PC (CurAddr)
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
      xassert(false && "Unknown store type!");
  }
  *_os << '\t';


  llvm::Value* rs1 = getReg(0);
  llvm::Value* rs2 = getReg(1);
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
      xassert(false && "Unknown store type!");
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
  switch (op) {
    case RW:
      xassert(false && "No CSR write support for base I instructions!");
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
  uint64_t mask;
  if (imm)
    mask = XRegister::A0; // Inst.getOperand(2).getImm();
  else
    mask = getRegNum(2);
  xassert(mask == XRegister::ZERO &&
    "No CSR write support for base I instructions!");
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
      xassert(false && "Not implemented!");
      break;
  }

  _bld->store(v, rd);

  return llvm::Error::success();
}


llvm::Error Instruction::translateBranch(BranchType bt)
{
  // Inst print
  switch (bt) {
    case JAL:   *_os << "jal";  break;
    case JALR:  *_os << "jalr"; break;
    case BEQ:   *_os << "beq";  break;
    case BNE:   *_os << "bne";  break;
    case BGE:   *_os << "bge";  break;
    case BGEU:  *_os << "bgeu";  break;
    case BLT:   *_os << "blt";  break;
    case BLTU:  *_os << "bltu"; break;
  }
  *_os << '\t';

  llvm::Error err = noError();

  // Get Operands
  unsigned o0n = getRegNum(0);
  llvm::Value* o0 = nullptr;
  unsigned o1n = 0;
  llvm::Value* o1 = nullptr;
  llvm::Value* o2 = nullptr;
  llvm::Value* jreg = nullptr;
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
      auto sos = _os;
      _os = &llvm::nulls();
      o1n = getRegNum(1);
      _os = sos;
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
    case BLTU:
      o0 = getReg(0);
      o1 = getReg(1);
      if (!exp(getImm(2), o2, err))
        return err;
      jimm = o2;
      break;
  }

  int64_t jimmi = 0;
  bool jimmIsZeroImm = false;
  bool isSymbol = _ctx->reloc->isSymbol(_addr);
  if (llvm::isa<llvm::ConstantInt>(jimm)) {
    jimmi = llvm::cast<llvm::ConstantInt>(jimm)->getValue().getSExtValue();
    jimmIsZeroImm = jimmi == 0 && !isSymbol;
  }
  llvm::Value* v = nullptr;

  // return?
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

  // Get target
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
    linkReg == XRegister::RA;
  _ctx->brWasLast = !isCall;
  const SBTSymbol& sym = _ctx->reloc->last();

  // JALR
  //
  // TODO Set Target LSB to zero
  if (bt == JALR) {
    // No base reg
    if (o1n == XRegister::ZERO) {
      xassert(false && "Unexpected JALR with base register equal to zero!");

    // No immediate
    } else if (jimmIsZeroImm) {
      v = jreg;
      if (isCall)
        act = ICALL;
      else
        act = IJUMP;

    // Base + Offset
    } else {
      v = _bld->add(jreg, jimm);
      if (isCall)
        act = ICALL;
      else
        act = IJUMP;
    }

  // JAL to Symbol
  } else if (bt == JAL && isSymbol) {
    if (isCall)
      act = CALL_SYMBOL;
    else
      act = JUMP_TO_SYMBOL;

  // JAL/Branches to PC offsets
  //
  // Add PC
  } else {
    target = jimmi + _addr;
    if (isCall)
      act = CALL_OFFS;
    else
      act = JUMP_TO_OFFS;
  }

  if (act == CALL_SYMBOL || act == JUMP_TO_SYMBOL) {
    if (sym.isExternal()) {
      v = jimm;
      xassert(isCall && "Jump to external module!");
      act = CALL_EXT;
    } else {
      xassert(sym.sec && sym.sec->isText() && "Jump to non Text Section!");
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

  switch (act) {
    case JUMP_TO_SYMBOL:
      xassert(false && "JUMP_TO_SYMBOL should become JUMP_TO_OFFS");
    case CALL_SYMBOL:
      xassert(false && "CALL_SYMBOL should become CALL_OFFS");

    case CALL_OFFS:
      if (auto err = handleCall(target))
        return err;
      break;

    case JUMP_TO_OFFS:
      if (auto err = handleJumpToOffs(target, cond, linkReg))
        return err;
      break;

    case ICALL:
      if (auto err = handleICall(v))
        return err;
      break;

    case CALL_EXT:
      if (auto err = handleCallExt(v))
        return err;
      break;

    case IJUMP:
      if (auto err = handleIJump(v, linkReg))
        return err;
      break;
  }

  return llvm::Error::success();
}


llvm::Error Instruction::handleCall(uint64_t target)
{
  // find function
  Function* f = Function::getByAddr(_ctx, target);
  _bld->call(f->func());
  return llvm::Error::success();
}


llvm::Error Instruction::handleICall(llvm::Value* target)
{
  _bld->store(target, XRegister::T1);
  _bld->call(_ctx->translator->icaller().func());
  return llvm::Error::success();
}


llvm::Error Instruction::handleCallExt(llvm::Value* target)
{
  llvm::FunctionType* ft = _t->voidFunc;
  llvm::Value* v = _bld->intToPtr(target, ft->getPointerTo());
  v = _bld->call(v);
  return llvm::Error::success();
}


llvm::Error Instruction::handleJumpToOffs(
  uint64_t target,
  llvm::Value* cond,
  unsigned linkReg)
{
  // get current function
  Function* f = _ctx->f;

  // next BB
  uint64_t nextInstrAddr = _addr + Instruction::SIZE;

  // link
  bool isCall = linkReg != XRegister::ZERO;
  if (!cond && isCall)
    _bld->store(_c->i32(nextInstrAddr), linkReg);

  // target BB
  xassert(target != _addr && "Unexpected jump to self instruction!");

  BasicBlock* bb = nullptr;
  auto& bbmap = f->bbmap();
  auto it = bbmap.lower_bound(target);
  bool needToTranslateBB = false;
  BasicBlock* entryBB = &bbmap.begin()->val;
  BasicBlock* predBB = nullptr;
  uint64_t bbEnd = 0;

  // jump forward
  if (target > _addr) {
    BasicBlock* beforeBB = nullptr;
    if (it != bbmap.end())
      beforeBB = &it->val;

    // BB already exists
    if (target == it->key)
      bb = &it->val;
    // need to create new BB
    else {
      bbmap(target, BasicBlock(_ctx, target, f, beforeBB));
      f->updateNextBB(target);
      bb = bbmap[target];
    }

  // jump backward
  } else {
    if (it == bbmap.end()) {
      xassert(!bbmap.empty() && "BBMap is empty!");
      bb = &(--it)->val;
    // BB entry matches target address
    } else if (target == it->key)
      bb = &it->val;
    // target BB is probably the previous one
    else if (it != bbmap.begin())
      bb = &(--it)->val;
    // target BB is the first one
    else
      bb = &it->val;

    // check bounds
    uint64_t begin = it->key;
    uint64_t end = it->key + bb->bb()->size() * Instruction::SIZE;
    bool inBound = target >= begin && target < end;

    // need to translate target
    if (!inBound) {
      bbEnd = it->key;
      predBB = &it->val;
      bbmap(target, BasicBlock(_ctx, target, f, bb));
      bb = bbmap[target];
      needToTranslateBB = true;
    // need to split BB?
    } else if (it->key != target) {
      bbmap(target, bb->split(target));
      bb = bbmap[target];
    }
  }

  // need NextBB?
  bool needNextBB = !isCall;

  BasicBlock* beforeBB = nullptr;
  BasicBlock* next = nullptr;
  if (needNextBB) {
    auto p = bbmap[nextInstrAddr];
    if (p)
      next = p;
    else {
      it = bbmap.lower_bound(nextInstrAddr);
      if (it != bbmap.end())
        beforeBB = &it->val;

      bbmap(nextInstrAddr, BasicBlock(_ctx, nextInstrAddr, f, beforeBB));
      next = bbmap[nextInstrAddr];
    }

    f->updateNextBB(nextInstrAddr);
  }

  // branch
  if (cond)
    _bld->condBr(cond, bb, next);
  else
    _bld->br(bb);

  // use next BB
  if (needNextBB)
    _bld->setInsertPoint(*next);

  // need to translate target BB?
  if (needToTranslateBB) {
    DBGS << "NeedToTranslateBB\n";

    // save insert point
    BasicBlock tempBB = _bld->getInsertBlock();
    BasicBlock* prevBB = &tempBB;

    // add branch to correct function entry BB
    if (entryBB->name() != "entry") {
      bbmap(0, BasicBlock(_ctx, "entry", f->func(), bb->bb()));
      BasicBlock* newEntryBB = bbmap[0];
      _bld->setInsertPoint(*newEntryBB);
      _bld->br(entryBB);
    }

    // translate BB
    _bld->setInsertPoint(*bb);
    // translate up to the next BB
    if (auto err = f->translateInstrs(target, bbEnd))
      return err;

    // link to the next BB if there is no terminator
    DBGS << "XBB=" << _bld->getInsertBlock().name() << nl;
    DBGS << "TBB=" << predBB->name() << nl;
    if (_bld->getInsertBlock().bb()->getTerminator() == nullptr)
      _bld->br(predBB);
    _ctx->brWasLast = true;

    // restore insert point
    _bld->setInsertPoint(*prevBB);
  }

  DBGS << "BB=" << _bld->getInsertBlock().name() << nl;
  return llvm::Error::success();
}


llvm::Error Instruction::handleIJump(llvm::Value* target, unsigned linkReg)
{
  xassert(false && "IJump support not implemented yet!");
}


#if SBT_DEBUG
// MD: metadata
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
  _bld->first()->setMetadata(getMDName(_ss->str()), n);
}

#else
void Instruction::dbgprint()
{}
#endif

}
