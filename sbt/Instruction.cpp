#include "Instruction.h"

#include "Builder.h"
#include "Context.h"
#include "Disassembler.h"
#include "Relocation.h"
#include "SBTError.h"

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
      err = handleSyscall();
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
  return llvm::Error::success();
}


llvm::Error Instruction::translateLoad(IntType it)
{
  return llvm::Error::success();
}


llvm::Error Instruction::translateStore(IntType it)
{
  return llvm::Error::success();
}


llvm::Error Instruction::translateBranch(BranchType bt)
{
  return llvm::Error::success();
}


llvm::Error Instruction::handleSyscall()
{
  return llvm::Error::success();
}


llvm::Error Instruction::translateFence(bool fi)
{
  return llvm::Error::success();
}


llvm::Error Instruction::translateCSR(CSROp op, bool imm)
{
  return llvm::Error::success();
}


#if 0

Error Translator::handleSyscall()
{
  Value *SC = load(RV_A7);
  std::vector<Value *> Args = { SC };
  Value *V = Builder->CreateCall(FRVSC, Args);
  updateFirst(V);
  store(V, RV_A0);

  return Error::success();
}


Error Translator::translateLoad(
  const llvm::MCInst &Inst,
  IntType IT,
  llvm::raw_string_ostream &SS)
{
  switch (IT) {
    case S8:
      ss << "lb";
      break;

    case U8:
      ss << "lbu";
      break;

    case S16:
      ss << "lh";
      break;

    case U16:
      ss << "lhu";
      break;

    case U32:
      ss << "lw";
      break;
  }
  ss << '\t';

  unsigned O = getRD(Inst, ss);
  Value *RS1 = getReg(Inst, 1, ss);
  auto ExpImm = getImm(Inst, 2, ss);
  if (!ExpImm)
    return ExpImm.takeError();
  Value *Imm = ExpImm.get();

  Value *V = add(RS1, Imm);

  Value *Ptr;
  switch (IT) {
    case S8:
    case U8:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I8Ptr);
      break;

    case S16:
    case U16:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I16Ptr);
      break;

    case U32:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I32Ptr);
      break;
  }
  updateFirst(Ptr);

  V = Builder->CreateLoad(Ptr);
  updateFirst(V);

  // to int32
  switch (IT) {
    case S8:
    case S16:
      V = Builder->CreateSExt(V, I32);
      updateFirst(V);
      break;

    case U8:
    case U16:
      V = Builder->CreateZExt(V, I32);
      updateFirst(V);
      break;

    case U32:
      break;
  }
  store(V, O);

  return Error::success();
}


Error Translator::translateStore(
  const llvm::MCInst &Inst,
  IntType IT,
  llvm::raw_string_ostream &SS)
{
  switch (IT) {
    case U8:
      ss << "sb";
      break;

    case U16:
      ss << "sh";
      break;

    case U32:
      ss << "sw";
      break;

    default:
      llvm_unreachable("Unknown store type!");
  }
  ss << '\t';


  Value *RS1 = getReg(Inst, 0, ss);
  Value *RS2 = getReg(Inst, 1, ss);
  auto ExpImm = getImm(Inst, 2, ss);
  if (!ExpImm)
    return ExpImm.takeError();
  Value *Imm = ExpImm.get();

  Value *V = add(RS1, Imm);

  Value *Ptr;
  switch (IT) {
    case U8:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I8Ptr);
      updateFirst(Ptr);
      V = Builder->CreateTruncOrBitCast(RS2, I8);
      updateFirst(V);
      break;

    case U16:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I16Ptr);
      updateFirst(Ptr);
      V = Builder->CreateTruncOrBitCast(RS2, I16);
      updateFirst(V);
      break;

    case U32:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I32Ptr);
      updateFirst(Ptr);
      V = RS2;
      break;

    default:
      llvm_unreachable("Unknown store type!");
  }

  V = Builder->CreateStore(V, Ptr);
  updateFirst(V);

  return Error::success();
}


llvm::Error Translator::translateBranch(
  const llvm::MCInst &Inst,
  BranchType BT,
  llvm::raw_string_ostream &SS)
{
  // Inst print
  switch (BT) {
    case JAL:   ss << "jal";  break;
    case JALR:  ss << "jalr"; break;
    case BEQ:   ss << "beq";  break;
    case BNE:   ss << "bne";  break;
    case BGE:   ss << "bge";  break;
    case BGEU:  ss << "bgeu";  break;
    case BLT:   ss << "blt";  break;
    case BLTU:  ss << "bltu"; break;
  }
  ss << '\t';

  Error E = noError();

  // Get Operands
  unsigned O0N = getRegNum(0, Inst, ss);
  Value *O0 = nullptr;
  unsigned O1N = 0;
  Value *O1 = nullptr;
  Value *O2 = nullptr;
  Value *JReg = nullptr;
  Value *JImm = nullptr;
  unsigned LinkReg = 0;
  switch (BT) {
    case JAL:
      LinkReg = O0N;
      if (!exp(getImm(Inst, 1, ss), O1, E))
        return E;
      JImm = O1;
      break;

    case JALR:
      LinkReg = O0N;
      O1N = getRegNum(1, Inst, nulls());
      O1 = getReg(Inst, 1, ss);
      if (!exp(getImm(Inst, 2, ss), O2, E))
        return E;
      JReg = O1;
      JImm = O2;
      break;

    case BEQ:
    case BNE:
    case BGE:
    case BGEU:
    case BLT:
    case BLTU:
      O0 = getReg(Inst, 0, ss);
      O1 = getReg(Inst, 1, ss);
      if (!exp(getImm(Inst, 2, ss), O2, E))
        return E;
      JImm = O2;
      break;
  }

  int64_t JImmI = 0;
  bool JImmIsZeroImm = false;
  if (isa<ConstantInt>(JImm)) {
    JImmI = cast<ConstantInt>(JImm)->getValue().getSExtValue();
    JImmIsZeroImm = JImmI == 0 && !LastImm.IsSym;
  }
  Value *V = nullptr;

  // Return?
  if (BT == JALR &&
      O0N == RV_ZERO &&
      O1N == RV_RA &&
      JImmIsZeroImm)
  {
    if (InMain)
      V = Builder->CreateRet(load(RV_A0));
    else
      V = Builder->CreateRetVoid();
    updateFirst(V);
    return Error::success();
  }

  // Get target
  uint64_t Target = 0;
  bool IsSymbol = LastImm.IsSym;
  SymbolReloc SR;

  enum Action {
    JUMP_TO_SYMBOL,
    JUMP_TO_OFFS,
    IJUMP,
    CALL_SYMBOL,
    CALL_OFFS,
    CALL_EXT,
    ICALL
  };
  Action Act;

  bool IsCall = (BT == JAL || BT == JALR) &&
    LinkReg == RV_RA;
  BrWasLast = !IsCall;

  // JALR
  //
  // TODO Set Target LSB to zero
  if (BT == JALR) {
    // No base reg
    if (O1N == RV_ZERO) {
      llvm_unreachable("Unexpected JALR with base register equal to zero!");

    // No immediate
    } else if (JImmIsZeroImm) {
      V = JReg;
      if (IsCall)
        Act = ICALL;
      else
        Act = IJUMP;

    // Base + Offset
    } else {
      V = Builder->CreateAdd(JReg, JImm);
      updateFirst(V);
      if (IsCall)
        Act = ICALL;
      else
        Act = IJUMP;
    }

  // JAL to Symbol
  } else if (BT == JAL && IsSymbol) {
    SR = LastImm.SymRel;
    if (IsCall)
      Act = CALL_SYMBOL;
    else
      Act = JUMP_TO_SYMBOL;

  // JAL/Branches to PC offsets
  //
  // Add PC
  } else {
    Target = JImmI + CurAddr;
    if (IsCall)
      Act = CALL_OFFS;
    else
      Act = JUMP_TO_OFFS;
  }

  if (Act == CALL_SYMBOL || Act == JUMP_TO_SYMBOL) {
    if (SR.isExternal()) {
      V = JImm;
      assert(IsCall && "Jump to external module!");
      Act = CALL_EXT;
    } else {
      assert(SR.Sec && SR.Sec->isText() && "Jump to non Text Section!");
      Target = SR.Addr;
      if (IsCall)
        Act = CALL_OFFS;
      else
        Act = JUMP_TO_OFFS;
    }
  }

  // Evaluate condition
  Value *Cond = nullptr;
  switch (BT) {
    case BEQ:
      Cond = Builder->CreateICmpEQ(O0, O1);
      break;

    case BNE:
      Cond = Builder->CreateICmpNE(O0, O1);
      break;

    case BGE:
      Cond = Builder->CreateICmpSGE(O0, O1);
      break;

    case BGEU:
      Cond = Builder->CreateICmpUGE(O0, O1);
      break;

    case BLT:
      Cond = Builder->CreateICmpSLT(O0, O1);
      break;

    case BLTU:
      Cond = Builder->CreateICmpULT(O0, O1);
      break;

    case JAL:
    case JALR:
      break;
  }
  if (Cond)
    updateFirst(Cond);

  switch (Act) {
    case JUMP_TO_SYMBOL:
      llvm_unreachable("JUMP_TO_SYMBOL should become JUMP_TO_OFFS");
    case CALL_SYMBOL:
      llvm_unreachable("CALL_SYMBOL should become CALL_OFFS");

    case CALL_OFFS:
      if (auto E = handleCall(Target))
        return E;
      break;

    case JUMP_TO_OFFS:
      if (auto E = handleJumpToOffs(Target, Cond, LinkReg))
        return E;
      break;

    case ICALL:
      if (auto E = handleICall(V))
        return E;
      break;

    case CALL_EXT:
      if (auto E = handleCallExt(V))
        return E;
      break;

    case IJUMP:
      if (auto E = handleIJump(V, LinkReg))
        return E;
      break;
  }

  return Error::success();
}


llvm::Error Translator::handleCall(uint64_t Target)
{
  // Find function
  // Get symbol by offset
  ConstSectionPtr Sec = CurObj->lookupSection(".text");
  assert(Sec && ".text section not found!");
  ConstSymbolPtr Sym = Sec->lookup(Target);
  assert(Sym &&
      (Sym->type() == llvm::object::SymbolRef::ST_Function ||
        (Sym->flags() & llvm::object::SymbolRef::SF_Global)) &&
    "Target function not found!");

  auto ExpF = createFunction(Sym->name());
  if (!ExpF)
    return ExpF.takeError();
  Function *F = ExpF.get();
  Value *V = Builder->CreateCall(F);
  updateFirst(V);
  return Error::success();
}


llvm::Error Translator::handleICall(llvm::Value *Target)
{
  store(Target, RV_T1);
  Value *V = Builder->CreateCall(ICaller);
  updateFirst(V);

  return Error::success();
}


llvm::Error Translator::handleCallExt(llvm::Value *Target)
{
  FunctionType *FT = FunctionType::get(Builder->getVoidTy(), !VAR_ARG);
  Value *V = Builder->CreateIntToPtr(Target, FT->getPointerTo());
  updateFirst(V);
  V = Builder->CreateCall(V);
  updateFirst(V);

  return Error::success();
}


llvm::Error Translator::handleJumpToOffs(
  uint64_t Target,
  llvm::Value *Cond,
  unsigned LinkReg)
{
  // Get current function
  const Function *CF = Builder->GetInsertBlock()->getParent();
  Function *F = Module->getFunction(CF->getName());

  // Next BB
  uint64_t NextInstrAddr = CurAddr + 4;

  // Link
  bool IsCall = LinkReg != RV_ZERO;
  if (!Cond && IsCall)
    store(ConstantInt::get(I32, NextInstrAddr), LinkReg);

  // Target BB
  assert(Target != CurAddr && "Unexpected jump to self instruction!");

  BasicBlock *BB = nullptr;
  auto Iter = BBMap.lower_bound(Target);
  bool NeedToTranslateBB = false;
  BasicBlock* entryBB = &F->getEntryBlock();
  BasicBlock* PredBB = nullptr;
  uint64_t BBEnd = 0;

  // Jump forward
  if (Target > CurAddr) {
    BasicBlock *BeforeBB = nullptr;
    if (Iter != BBMap.end())
      BeforeBB = Iter->val;

    // BB already exists
    if (Target == Iter->key)
      BB = Iter->val;
    // Need to create new BB
    else {
      BB = SBTBasicBlock::create(*Context, Target, F, BeforeBB);
      BBMap(Target, std::move(BB));

      updateNextBB(Target);
    }

  // jump backward
  } else {
    if (Iter == BBMap.end()) {
      assert(!BBMap.empty() && "BBMap is empty!");
      BB = (--Iter)->val;
    // BB entry matches target address
    } else if (Target == Iter->key)
      BB = Iter->val;
    // target BB is probably the previous one
    else if (Iter != BBMap.begin())
      BB = (--Iter)->val;
    // target BB is the first one
    else
      BB = Iter->val;

    // check bounds
    uint64_t Begin = Iter->key;
    uint64_t End = Iter->key + BB->size() * InstrSize;
    bool InBound = Target >= Begin && Target < End;

    // need to translate target
    if (!InBound) {
      BBEnd = Iter->key;
      PredBB = Iter->val;
      BB = SBTBasicBlock::create(*Context, Target, F, BB);
      BBMap(Target, std::move(BB));
      NeedToTranslateBB = true;
    // need to split BB?
    } else if (Iter->key != Target)
      BB = splitBB(BB, Target);
  }

  // need NextBB?
  bool NeedNextBB = !IsCall;

  BasicBlock *BeforeBB = nullptr;
  BasicBlock *Next = nullptr;
  if (NeedNextBB) {
    auto P = BBMap[NextInstrAddr];
    if (P)
      Next = *P;
    else {
      Iter = BBMap.lower_bound(NextInstrAddr);
      if (Iter != BBMap.end())
        BeforeBB = Iter->val;

      Next = SBTBasicBlock::create(*Context, NextInstrAddr, F, BeforeBB);
      BBMap(NextInstrAddr, std::move(Next));
    }

    updateNextBB(NextInstrAddr);
  }

  // Branch
  Value *V;
  if (Cond)
    V = Builder->CreateCondBr(Cond, BB, Next);
  else
    V = Builder->CreateBr(BB);
  updateFirst(V);

  // Use next BB
  if (NeedNextBB)
    Builder->SetInsertPoint(Next);

  // need to translate target BB?
  if (NeedToTranslateBB) {
    DBGS << "NeedToTranslateBB\n";

    // save insert point
    BasicBlock* PrevBB = Builder->GetInsertBlock();

    // add branch to correct function entry BB
    if (entryBB->getName() != "entry") {
      BasicBlock* newEntryBB =
        SBTBasicBlock::create(*Context, "entry", F, BB);
      Builder->SetInsertPoint(newEntryBB);
      Builder->CreateBr(entryBB);
    }

    // translate BB
    Builder->SetInsertPoint(BB);
    // translate up to the next BB
    if (auto E = translateInstrs(Target, BBEnd))
      return E;

    // link to the next BB if there is no terminator
    DBGS << "XBB=" << Builder->GetInsertBlock()->getName() << nl;
    DBGS << "TBB=" << PredBB->getName() << nl;
    if (Builder->GetInsertBlock()->getTerminator() == nullptr)
      Builder->CreateBr(PredBB);
    BrWasLast = true;

    // restore insert point
    Builder->SetInsertPoint(PrevBB);
  }

  DBGS << "BB=" << Builder->GetInsertBlock()->getName() << nl;
  return Error::success();
}

llvm::Error Translator::handleIJump(
  llvm::Value *Target,
  unsigned LinkReg)
{
  llvm_unreachable("IJump support not implemented yet!");
}

llvm::Error Translator::translateUI(
  const llvm::MCInst &Inst,
  UIOp UOP,
  llvm::raw_string_ostream &SS)
{
  switch (UOP) {
    case AUIPC: ss << "auipc";  break;
    case LUI:   ss << "lui";    break;
  }
  ss << '\t';

  unsigned O = getRD(Inst, ss);
  auto ExpImm = getImm(Inst, 1, ss);
  if (!ExpImm)
    return ExpImm.takeError();
  Value *Imm = ExpImm.get();
  Value *V;

  if (LastImm.IsSym)
    V = Imm;
  else {
    // get upper immediate
    V = Builder->CreateShl(Imm, ConstantInt::get(I32, 12));
    updateFirst(V);

    // Add PC (CurAddr)
    if (UOP == AUIPC) {
      V = add(V, ConstantInt::get(I32, CurAddr));
      updateFirst(V);
    }
  }

  store(V, O);

  return Error::success();
}


llvm::Error Translator::translateFence(
  const llvm::MCInst &Inst,
  bool FI,
  llvm::raw_string_ostream &SS)
{
  if (FI) {
    ss << "fence.i";
    nop();
    return Error::success();
  }

  ss << "fence";
  AtomicOrdering Order = llvm::AtomicOrdering::AcquireRelease;
  SynchronizationScope Scope = llvm::SynchronizationScope::CrossThread;

  Value *V;
  V = Builder->CreateFence(Order, Scope);
  updateFirst(V);

  return Error::success();
}

llvm::Error Translator::translateCSR(
  const llvm::MCInst &Inst,
  CSROp Op,
  bool Imm,
  llvm::raw_string_ostream &SS)
{
  switch (Op) {
    case RW:
      assert(false && "No CSR write support for base I instructions!");
      break;

    case RS:
      ss << "csrrs";
      break;

    case RC:
      ss << "csrrc";
      break;
  }
  if (Imm)
    ss << "i";
  ss << '\t';

  unsigned RD = getRegNum(0, Inst, ss);
  uint64_t CSR = Inst.getOperand(1).getImm();
  uint64_t Mask;
  if (Imm)
    Mask = RV_A0; // Inst.getOperand(2).getImm();
  else
    Mask = getRegNum(2, Inst, ss);
  assert(Mask == RV_ZERO && "No CSR write support for base I instructions!");
  ss << llvm::formatv("0x{0:X-4} = ", CSR);

  Value *V = ConstantInt::get(I32, 0);
  switch (CSR) {
    case RDCYCLE:
      ss << "RDCYCLE";
      V = Builder->CreateCall(GetCycles);
      updateFirst(V);
      break;
    case RDCYCLEH:
      ss << "RDCYCLEH";
      break;
    case RDTIME:
      ss << "RDTIME";
      V = Builder->CreateCall(GetTime);
      updateFirst(V);
      break;
    case RDTIMEH:
      ss << "RDTIMEH";
      break;
    case RDINSTRET:
      ss << "RDINSTRET";
      V = Builder->CreateCall(InstRet);
      updateFirst(V);
      break;
    case RDINSTRETH:
      ss << "RDINSTRETH";
      break;
    default:
      assert(false && "Not implemented!");
      break;
  }

  store(V, RD);
  return Error::success();
}

#endif


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
