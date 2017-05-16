#include "Instruction.h"

#include "Context.h"
#include "Disassembler.h"
#include "SBTError.h"

#include <llvm/MC/MCInst.h>
#include <llvm/Support/FormatVariadic.h>

// LLVM internal instruction info
#define GET_INSTRINFO_ENUM
#include <llvm/Target/RISCVMaster/RISCVMasterGenInstrInfo.inc>


namespace sbt {

llvm::Error Instruction::translate()
{
  llvm::MCInst inst;
  size_t size;
  if (auto err = _ctx->disasm->disasm(_addr, _rawInst, inst, size))
    return err;

  return llvm::Error::success();
}

}

#if 0


llvm::Error Translator::translate(const llvm::MCInst &Inst)
{
  namespace RISCV = RISCVMaster;

  SBTError SE;
  First = nullptr;

#if SBT_DEBUG
  std::string SSS;
  raw_string_ostream SS(SSS);
#else
  raw_ostream &SS = nulls();
#endif
  SS << formatv("{0:X-4}:   ", CurAddr);

  if (NextBB && CurAddr == NextBB) {
    BasicBlock **BB = BBMap[CurAddr];
    assert(BB && "BasicBlock not found!");
    if (!BrWasLast)
      Builder->CreateBr(*BB);
    Builder->SetInsertPoint(*BB);

    auto Iter = BBMap.lower_bound(CurAddr + 4);
    if (Iter != BBMap.end())
      updateNextBB(Iter->key);
  }

  Error E = noError();
  BrWasLast = false;

  switch (Inst.getOpcode())
  {
    // ALU Ops
    case RISCV::ADD:
      E = translateALUOp(Inst, ADD, AF_NONE, SS);
      break;
    case RISCV::ADDI:
      E = translateALUOp(Inst, ADD, AF_IMM, SS);
      break;
    case RISCV::AND:
      E = translateALUOp(Inst, AND, AF_NONE, SS);
      break;
    case RISCV::ANDI:
      E = translateALUOp(Inst, AND, AF_IMM, SS);
      break;
    case RISCV::MUL:
      E = translateALUOp(Inst, MUL, AF_NONE, SS);
      break;
    case RISCV::OR:
      E = translateALUOp(Inst, OR, AF_NONE, SS);
      break;
    case RISCV::ORI:
      E = translateALUOp(Inst, OR, AF_IMM, SS);
      break;
    case RISCV::SLL:
      E = translateALUOp(Inst, SLL, AF_NONE, SS);
      break;
    case RISCV::SLLI:
      E = translateALUOp(Inst, SLL, AF_IMM, SS);
      break;
    case RISCV::SLT:
      E = translateALUOp(Inst, SLT, AF_NONE, SS);
      break;
    case RISCV::SLTU:
      E = translateALUOp(Inst, SLT, AF_UNSIGNED, SS);
      break;
    case RISCV::SLTI:
      E = translateALUOp(Inst, SLT, AF_IMM, SS);
      break;
    case RISCV::SLTIU:
      E = translateALUOp(Inst, SLT, AF_IMM | AF_UNSIGNED, SS);
      break;
    case RISCV::SRA:
      E = translateALUOp(Inst, SRA, AF_NONE, SS);
      break;
    case RISCV::SRAI:
      E = translateALUOp(Inst, SRA, AF_IMM, SS);
      break;
    case RISCV::SRL:
      E = translateALUOp(Inst, SRL, AF_NONE, SS);
      break;
    case RISCV::SRLI:
      E = translateALUOp(Inst, SRL, AF_IMM, SS);
      break;
    case RISCV::SUB:
      E = translateALUOp(Inst, SUB, AF_NONE, SS);
      break;
    case RISCV::XOR:
      E = translateALUOp(Inst, XOR, AF_NONE, SS);
      break;
    case RISCV::XORI:
      E = translateALUOp(Inst, XOR, AF_IMM, SS);
      break;

    // UI
    case RISCV::AUIPC:
      E = translateUI(Inst, AUIPC, SS);
      break;
    case RISCV::LUI:
      E = translateUI(Inst, LUI, SS);
      break;

    // Branch
    case RISCV::BEQ:
      E = translateBranch(Inst, BEQ, SS);
      break;
    case RISCV::BNE:
      E = translateBranch(Inst, BNE, SS);
      break;
    case RISCV::BGE:
      E = translateBranch(Inst, BGE, SS);
      break;
    case RISCV::BGEU:
      E = translateBranch(Inst, BGEU, SS);
      break;
    case RISCV::BLT:
      E = translateBranch(Inst, BLT, SS);
      break;
    case RISCV::BLTU:
      E = translateBranch(Inst, BLTU, SS);
      break;
    // Jump
    case RISCV::JAL:
      E = translateBranch(Inst, JAL, SS);
      break;
    case RISCV::JALR:
      E = translateBranch(Inst, JALR, SS);
      break;

    // ecall
    case RISCV::ECALL:
      SS << "ecall";
      E = handleSyscall();
      break;

    // ebreak
    case RISCV::EBREAK:
      SS << "ebreak";
      nop();
      break;

    // Load
    case RISCV::LB:
      E = translateLoad(Inst, S8, SS);
      break;
    case RISCV::LBU:
      E = translateLoad(Inst, U8, SS);
      break;
    case RISCV::LH:
      E = translateLoad(Inst, S16, SS);
      break;
    case RISCV::LHU:
      E = translateLoad(Inst, U16, SS);
      break;
    case RISCV::LW:
      E = translateLoad(Inst, U32, SS);
      break;

    // Store
    case RISCV::SB:
      E = translateStore(Inst, U8, SS);
      break;
    case RISCV::SH:
      E = translateStore(Inst, U16, SS);
      break;
    case RISCV::SW:
      E = translateStore(Inst, U32, SS);
      break;

    // fence
    case RISCV::FENCE:
      E = translateFence(Inst, false, SS);
      break;
    case RISCV::FENCEI:
      E = translateFence(Inst, true, SS);
      break;

    // system
    case RISCV::CSRRW:
      E = translateCSR(Inst, RW, false, SS);
      break;
    case RISCV::CSRRWI:
      E = translateCSR(Inst, RW, true, SS);
      break;
    case RISCV::CSRRS:
      E = translateCSR(Inst, RS, false, SS);
      break;
    case RISCV::CSRRSI:
      E = translateCSR(Inst, RS, true, SS);
      break;
    case RISCV::CSRRC:
      E = translateCSR(Inst, RC, false, SS);
      break;
    case RISCV::CSRRCI:
      E = translateCSR(Inst, RC, true, SS);
      break;

    default:
      SE << "Unknown instruction opcode: " << Inst.getOpcode();
      return error(SE);
  }

  if (E)
    return E;

  if (!First)
    First = load(RV_A0);

  dbgprint(SS);
  InstrMap(CurAddr, std::move(First));
  return Error::success();
}

Error Translator::handleSyscall()
{
  Value *SC = load(RV_A7);
  std::vector<Value *> Args = { SC };
  Value *V = Builder->CreateCall(FRVSC, Args);
  updateFirst(V);
  store(V, RV_A0);

  return Error::success();
}

llvm::Error Translator::translateALUOp(
  const llvm::MCInst &Inst,
  ALUOp Op,
  uint32_t Flags,
  llvm::raw_string_ostream &SS)
{
  bool HasImm = Flags & AF_IMM;
  bool IsUnsigned = Flags & AF_UNSIGNED;

  switch (Op) {
    case ADD: SS << "add";  break;
    case AND: SS << "and";  break;
    case MUL: SS << "mul";  break;
    case OR:  SS << "or";   break;
    case SLL: SS << "sll";  break;
    case SLT: SS << "slt";  break;
    case SRA: SS << "sra";  break;
    case SRL: SS << "srl";  break;
    case SUB: SS << "sub";  break;
    case XOR: SS << "xor";  break;
  }
  if (HasImm)
    SS << "i";
  if (IsUnsigned)
    SS << "u";
  SS << '\t';

  unsigned O = getRD(Inst, SS);
  Value *O1 = getReg(Inst, 1, SS);
  Value *O2;
  if (HasImm) {
    auto ExpImm = getImm(Inst, 2, SS);
    if (!ExpImm)
      return ExpImm.takeError();
    O2 = ExpImm.get();
  } else
    O2 = getReg(Inst, 2, SS);

  Value *V;

  switch (Op) {
    case ADD:
      V = add(O1, O2);
      break;

    case AND:
      V = Builder->CreateAnd(O1, O2);
      break;

    case MUL:
      V = Builder->CreateMul(O1, O2);
      break;

    case OR:
      V = Builder->CreateOr(O1, O2);
      break;

    case SLL:
      V = Builder->CreateShl(O1, O2);
      break;

    case SLT:
      if (IsUnsigned)
        V = Builder->CreateICmpULT(O1, O2);
      else
        V = Builder->CreateICmpSLT(O1, O2);
      updateFirst(V);
      V = Builder->CreateZExt(V, I32);
      break;

    case SRA:
      V = Builder->CreateAShr(O1, O2);
      break;

    case SRL:
      V = Builder->CreateLShr(O1, O2);
      break;

    case SUB:
      V = Builder->CreateSub(O1, O2);
      break;

    case XOR:
      V = Builder->CreateXor(O1, O2);
      break;
  }
  updateFirst(V);

  store(V, O);

  return Error::success();
}


Error Translator::translateLoad(
  const llvm::MCInst &Inst,
  IntType IT,
  llvm::raw_string_ostream &SS)
{
  switch (IT) {
    case S8:
      SS << "lb";
      break;

    case U8:
      SS << "lbu";
      break;

    case S16:
      SS << "lh";
      break;

    case U16:
      SS << "lhu";
      break;

    case U32:
      SS << "lw";
      break;
  }
  SS << '\t';

  unsigned O = getRD(Inst, SS);
  Value *RS1 = getReg(Inst, 1, SS);
  auto ExpImm = getImm(Inst, 2, SS);
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
      SS << "sb";
      break;

    case U16:
      SS << "sh";
      break;

    case U32:
      SS << "sw";
      break;

    default:
      llvm_unreachable("Unknown store type!");
  }
  SS << '\t';


  Value *RS1 = getReg(Inst, 0, SS);
  Value *RS2 = getReg(Inst, 1, SS);
  auto ExpImm = getImm(Inst, 2, SS);
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
    case JAL:   SS << "jal";  break;
    case JALR:  SS << "jalr"; break;
    case BEQ:   SS << "beq";  break;
    case BNE:   SS << "bne";  break;
    case BGE:   SS << "bge";  break;
    case BGEU:  SS << "bgeu";  break;
    case BLT:   SS << "blt";  break;
    case BLTU:  SS << "bltu"; break;
  }
  SS << '\t';

  Error E = noError();

  // Get Operands
  unsigned O0N = getRegNum(0, Inst, SS);
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
      if (!exp(getImm(Inst, 1, SS), O1, E))
        return E;
      JImm = O1;
      break;

    case JALR:
      LinkReg = O0N;
      O1N = getRegNum(1, Inst, nulls());
      O1 = getReg(Inst, 1, SS);
      if (!exp(getImm(Inst, 2, SS), O2, E))
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
      O0 = getReg(Inst, 0, SS);
      O1 = getReg(Inst, 1, SS);
      if (!exp(getImm(Inst, 2, SS), O2, E))
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
    case AUIPC: SS << "auipc";  break;
    case LUI:   SS << "lui";    break;
  }
  SS << '\t';

  unsigned O = getRD(Inst, SS);
  auto ExpImm = getImm(Inst, 1, SS);
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
    SS << "fence.i";
    nop();
    return Error::success();
  }

  SS << "fence";
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
      SS << "csrrs";
      break;

    case RC:
      SS << "csrrc";
      break;
  }
  if (Imm)
    SS << "i";
  SS << '\t';

  unsigned RD = getRegNum(0, Inst, SS);
  uint64_t CSR = Inst.getOperand(1).getImm();
  uint64_t Mask;
  if (Imm)
    Mask = RV_A0; // Inst.getOperand(2).getImm();
  else
    Mask = getRegNum(2, Inst, SS);
  assert(Mask == RV_ZERO && "No CSR write support for base I instructions!");
  SS << llvm::formatv("0x{0:X-4} = ", CSR);

  Value *V = ConstantInt::get(I32, 0);
  switch (CSR) {
    case RDCYCLE:
      SS << "RDCYCLE";
      V = Builder->CreateCall(GetCycles);
      updateFirst(V);
      break;
    case RDCYCLEH:
      SS << "RDCYCLEH";
      break;
    case RDTIME:
      SS << "RDTIME";
      V = Builder->CreateCall(GetTime);
      updateFirst(V);
      break;
    case RDTIMEH:
      SS << "RDTIMEH";
      break;
    case RDINSTRET:
      SS << "RDINSTRET";
      V = Builder->CreateCall(InstRet);
      updateFirst(V);
      break;
    case RDINSTRETH:
      SS << "RDINSTRETH";
      break;
    default:
      assert(false && "Not implemented!");
      break;
  }

  store(V, RD);
  return Error::success();
}

#if SBT_DEBUG
static std::string getMDName(const llvm::StringRef &S)
{
  std::string SSS;
  raw_string_ostream SS(SSS);
  SS << '_';
  for (char C : S) {
    switch (C) {
      case ' ':
      case ':':
      case ',':
      case '%':
      case '(':
      case ')':
      case '\t':
        SS << '_';
        break;

      case '=':
        SS << "eq";
        break;

      default:
        SS << C;
    }
  }
  return SS.str();
}

void Translator::dbgprint(llvm::raw_string_ostream &SS)
{
  DBGS << SS.str() << "\n";
  MDNode *N = MDNode::get(*Context,
    MDString::get(*Context, "RISC-V Instruction"));
  First->setMetadata(getMDName(SS.str()), N);
}
#endif

#endif
