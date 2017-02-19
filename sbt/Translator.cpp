#include "Translator.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/MemoryBuffer.h>

// LLVM internal instruction info
#define GET_INSTRINFO_ENUM
#include <lib/Target/RISCVMaster/RISCVMasterGenInstrInfo.inc>
#define GET_REGINFO_ENUM
#include <lib/Target/RISCVMaster/RISCVMasterGenRegisterInfo.inc>

#include <algorithm>
#include <vector>

using namespace llvm;

namespace sbt {

Translator::Translator(
  LLVMContext *Ctx,
  IRBuilder<> *Bldr,
  llvm::Module *Mod)
  :
  I32(Type::getInt32Ty(*Ctx)),
  I16(Type::getInt32Ty(*Ctx)),
  I8(Type::getInt32Ty(*Ctx)),
  I32Ptr(Type::getInt32PtrTy(*Ctx)),
  I16Ptr(Type::getInt32PtrTy(*Ctx)),
  I8Ptr(Type::getInt8PtrTy(*Ctx)),
  ZERO(ConstantInt::get(I32, 0)),
  Context(Ctx),
  Builder(Bldr),
  Module(Mod)
{
}

// MCInst register number to RISCV register number
unsigned Translator::RVReg(unsigned Reg)
{
  namespace RISCV = RISCVMaster;

  switch (Reg) {
    case RISCV::X0_32:  return 0;
    case RISCV::X1_32:  return 1;
    case RISCV::X2_32:  return 2;
    case RISCV::X3_32:  return 3;
    case RISCV::X4_32:  return 4;
    case RISCV::X5_32:  return 5;
    case RISCV::X6_32:  return 6;
    case RISCV::X7_32:  return 7;
    case RISCV::X8_32:  return 8;
    case RISCV::X9_32:  return 9;
    case RISCV::X10_32: return 10;
    case RISCV::X11_32: return 11;
    case RISCV::X12_32: return 12;
    case RISCV::X13_32: return 13;
    case RISCV::X14_32: return 14;
    case RISCV::X15_32: return 15;
    case RISCV::X16_32: return 16;
    case RISCV::X17_32: return 17;
    case RISCV::X18_32: return 18;
    case RISCV::X19_32: return 19;
    case RISCV::X20_32: return 20;
    case RISCV::X21_32: return 21;
    case RISCV::X22_32: return 22;
    case RISCV::X23_32: return 23;
    case RISCV::X24_32: return 24;
    case RISCV::X25_32: return 25;
    case RISCV::X26_32: return 26;
    case RISCV::X27_32: return 27;
    case RISCV::X28_32: return 28;
    case RISCV::X29_32: return 29;
    case RISCV::X30_32: return 30;
    case RISCV::X31_32: return 31;
    default: return 0x1000;
  }
}

Error Translator::translate(const llvm::MCInst &Inst)
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
    BasicBlock **BB = BBS[CurAddr];
    assert(BB && "BasicBlock not found!");
    Builder->CreateBr(*BB);
    Builder->SetInsertPoint(*BB);
  }

  switch (Inst.getOpcode())
  {
    case RISCV::ADD: {
      SS << "add\t";

      unsigned O = getRD(Inst, SS);
      Value *O1 = getReg(Inst, 1, SS);
      Value *O2 = getReg(Inst, 2, SS);

      Value *V = add(O1, O2);
      store(V, O);

      resetRelInfo(O);
      dbgprint(SS);
      break;
    }

    case RISCV::ADDI: {
      SS << "addi\t";

      unsigned O = getRD(Inst, SS);
      Value *O1 = getReg(Inst, 1, SS);
      Value *O2 = getRegOrImm(Inst, 2, SS);

      Value *V = add(O1, O2);
      store(V, O);

      setRelInfo(O);
      dbgprint(SS);
      break;
    }

    case RISCV::AUIPC: {
      SS << "auipc\t";

      unsigned O = getRD(Inst, SS);
      Value *V = getImm(Inst, 1, SS);

      // Add PC (CurAddr) if V is not a relocation
      if (!isRelocation(V)) {
        V = add(V, ConstantInt::get(I32, CurAddr));
        // Get upper 20 bits only
        V = Builder->CreateAnd(V, ConstantInt::get(I32, 0xFFFFF000));
        updateFirst(V);
      }
      // Note: for relocations, the mask was already applied to the result

      store(V, O);

      setRelInfo(O);
      dbgprint(SS);
      break;
    }

    case RISCV::BEQ:
      if (auto E = translateBranch(Inst, BEQ, SS))
        return E;
      break;

    case RISCV::ECALL: {
      SS << "ecall";

      if (Error E = handleSyscall())
        return E;

      dbgprint(SS);
      break;
    }

    case RISCV::JALR: {
      SS << "jalr\t";

      unsigned RD = getRD(Inst, SS);
      unsigned RS1 = getRegNum(1, Inst, SS);
      Value *Imm = getImm(Inst, 2, SS);
      Value *V;

      // Check for 'return'
      if (RD == RV_ZERO &&
          RS1 == RV_RA &&
          isa<ConstantInt>(Imm) &&
          cast<ConstantInt>(Imm)->getValue() == 0)
      {
        if (InMain)
          V = Builder->CreateRet(load(RV_A0));
        else
          V = Builder->CreateRetVoid();
        updateFirst(V);

      // External call
      } else if (RD == RV_RA &&
          isa<ConstantInt>(Imm) &&
          cast<ConstantInt>(Imm)->getValue() == 0)
      {
        StringRef Sym = XSyms[RS1];
        if (!Sym.empty()) {
          auto ExpV = call(Sym);
          if (!ExpV)
            return ExpV.takeError();
          V = *ExpV;
          if (!V->getType()->isVoidTy())
            store(V, RV_A0);
        }

      // TODO Internal call
      } else {
        assert(false && "TODO implement Internal call");
        // Value *Target = add(V, Imm);
        // Target = Builder->CreateAnd(Target, ~1U);
        // updateFirst(Target);
      }

      dbgprint(SS);
      break;
    }

    case RISCV::LBU:
      if (auto E = translateLoad(Inst, U8, SS))
        return E;
      break;

    case RISCV::LW:
      if (auto E = translateLoad(Inst, U32, SS))
        return E;
      break;

    case RISCV::LUI: {
      SS << "lui\t";

      unsigned O = getRD(Inst, SS);
      Value *Imm = getImm(Inst, 1, SS);

      Value *V;
      if (IsRel)
        V = Imm;
      else {
        V = Builder->CreateShl(Imm, ConstantInt::get(I32, 12));
        updateFirst(V);
      }
      store(V, O);

      setRelInfo(O);
      dbgprint(SS);
      break;
    }

    case RISCV::SW: {
      SS << "sw\t";

      Value *RS1 = getReg(Inst, 0, SS);
      Value *RS2 = getReg(Inst, 1, SS);
      Value *Imm = getImm(Inst, 2, SS);

      Value *V = add(RS1, Imm);

      Value *Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I32Ptr);
      updateFirst(Ptr);
      V = Builder->CreateStore(RS2, Ptr);
      updateFirst(V);

      dbgprint(SS);
      break;
    }

    default: {
      SE << "Unknown instruction opcode: " << Inst.getOpcode();
      return error(SE);
    }
  }

  return Error::success();
}

Error Translator::startModule()
{
  if (auto E = declRegisterFile())
    return E;

  if (auto E = buildShadowImage())
    return E;

  if (auto E = buildStack())
    return E;

  declSyscallHandler();

  return Error::success();
}

Error Translator::genSCHandler()
{
  if (auto E = buildRegisterFile())
    return E;

  if (Error E = genSyscallHandler())
    return E;

  return Error::success();
}

Error Translator::finishModule()
{
  return Error::success();
}

Error Translator::startMain(StringRef Name, uint64_t Addr)
{
  // Create a function with no parameters
  FunctionType *FT =
    FunctionType::get(I32, !VAR_ARG);
  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, Module);

  // BB
  BasicBlock *BB = BasicBlock::Create(*Context, getBBName(Addr), F);
  Builder->SetInsertPoint(BB);

  // Set stack pointer.

  std::vector<Value *> Idx = { ZERO, ConstantInt::get(I32, StackSize) };
  Value *V =
    Builder->CreateGEP(Stack, Idx);
  StackEnd = i8PtrToI32(V);

  store(StackEnd, RV_SP);
  InMain = true;

  return Error::success();
}

Error Translator::startFunction(StringRef Name, uint64_t Addr)
{
  // Create a function with no parameters
  FunctionType *FT =
    FunctionType::get(Type::getVoidTy(*Context), !VAR_ARG);
  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, Module);

  // BB
  BasicBlock *BB = BasicBlock::Create(*Context, getBBName(Addr), F);
  Builder->SetInsertPoint(BB);

  return Error::success();
}

Error Translator::finishFunction()
{
  if (Builder->GetInsertBlock()->getTerminator() == nullptr)
    Builder->CreateRetVoid();
  InMain = false;
  return Error::success();
}

Error Translator::declOrBuildRegisterFile(bool decl)
{
  Constant *X0 = ConstantInt::get(I32, 0u);
  X[0] = new GlobalVariable(*Module, I32, CONSTANT,
    GlobalValue::ExternalLinkage, decl? nullptr : X0, IR_XREGNAME + "0");

  for (int I = 1; I < 32; ++I) {
    std::string S;
    raw_string_ostream SS(S);
    SS << IR_XREGNAME << I;
    X[I] = new GlobalVariable(*Module, I32, !CONSTANT,
        GlobalValue::ExternalLinkage, decl? nullptr : X0, SS.str());
  }

  return Error::success();
}

Error Translator::buildShadowImage()
{
  SBTError SE;

  std::vector<uint8_t> Vec;
  for (ConstSectionPtr Sec : CurObj->sections()) {
    // Skip non text/data sections
    if (!Sec->isText() && !Sec->isData())
      continue;

    // Read contents
    StringRef Bytes;
    std::error_code EC = Sec->contents(Bytes);
    if (EC) {
      SE  << __FUNCTION__ << ": failed to get section ["
          << Sec->name() << "] contents";
      return error(SE);
    }

    // Set Shadow Offset of Section
    Sec->shadowOffs(Vec.size());

    ArrayRef<uint8_t> ByteArray(
      reinterpret_cast<const uint8_t *>(Bytes.data()),
      Bytes.size());

    // Append to vector
    for (size_t I = 0; I < Bytes.size(); I++)
      Vec.push_back(Bytes[I]);
  }

  // Create the ShadowImage
  Constant *CDA = ConstantDataArray::get(*Context, Vec);
  ShadowImage =
    new GlobalVariable(*Module, CDA->getType(), !CONSTANT,
      GlobalValue::ExternalLinkage, CDA, "ShadowMemory");

  return Error::success();
}

void Translator::declSyscallHandler()
{
  FTRVSC = FunctionType::get(I32, { I32 }, !VAR_ARG);
  FRVSC =
    Function::Create(FTRVSC, Function::ExternalLinkage, "rv_syscall", Module);
}

Error Translator::genSyscallHandler()
{
  // Declare X86 syscall functions
  const size_t N = 5;
  FunctionType *FTX86SC[N];
  Function *FX86SC[N];
  std::vector<Type *> FArgs = { I32 };

  const std::string SCName = "syscall";
  for (size_t I = 0; I < N; I++) {
    std::string S = SCName;
    raw_string_ostream SS(S);
    SS << I;

    FTX86SC[I] = FunctionType::get(I32, FArgs, !VAR_ARG);
    FArgs.push_back(I32);

    FX86SC[I] = Function::Create(FTX86SC[I],
      Function::ExternalLinkage, SS.str(), Module);
  }

  // Build syscalls' info
  struct Syscall {
    int Args;
    int RV;
    int X86;

    Syscall(int A, int R, int X) :
      Args(A),
      RV(R),
      X86(X)
    {}
  };

  // Arg #, RISC-V Syscall #, X86 Syscall #
  const int X86_SYS_EXIT = 1;
  const std::vector<Syscall> SCV = {
    { 1, 93, 1 },  // EXIT
    { 3, 64, 4 }   // WRITE
  };

  const std::string BBPrefix = "bb_rvsc_";
  First = nullptr;

  declSyscallHandler();

  // Entry
  BasicBlock *BBEntry = BasicBlock::Create(*Context, BBPrefix + "entry", FRVSC);
  llvm::Argument &SC = *FRVSC->arg_begin();

  // Exit
  //
  // Return A0
  BasicBlock *BBExit = BasicBlock::Create(*Context,
    BBPrefix + "exit", FRVSC);
  Builder->SetInsertPoint(BBExit);
  Builder->CreateRet(load(RV_A0));

  // 2nd switch
  BasicBlock *BBSW2 = BasicBlock::Create(*Context,
    BBPrefix + "sw2", FRVSC, BBExit);

  // First Switch:
  // - switch based on RISC-V syscall number and:
  //   - set number of args
  //   - set X86 syscall number

  // Default case: call exit(99)
  BasicBlock *BBSW1Dfl = BasicBlock::Create(*Context,
    BBPrefix + "sw1_default", FRVSC, BBSW2);
  Builder->SetInsertPoint(BBSW1Dfl);
  store(ConstantInt::get(I32, 1), RV_T0);
  store(ConstantInt::get(I32, X86_SYS_EXIT), RV_A7);
  store(ConstantInt::get(I32, 99), RV_A0);
  Builder->CreateBr(BBSW2);

  Builder->SetInsertPoint(BBEntry);
  SwitchInst *SW1 = Builder->CreateSwitch(&SC, BBSW1Dfl);

  // Other cases
  auto addSW1Case = [&](const Syscall &S) {
    std::string SSS = BBPrefix;
    raw_string_ostream SS(SSS);
    SS << "sw1_case_" << S.RV;

    BasicBlock *BB = BasicBlock::Create(*Context, SS.str(), FRVSC, BBSW2);
    Builder->SetInsertPoint(BB);
    store(ConstantInt::get(I32, S.Args), RV_T0);
    store(ConstantInt::get(I32, S.X86), RV_A7);
    Builder->CreateBr(BBSW2);
    SW1->addCase(ConstantInt::get(I32, S.RV), BB);
  };

  for (const Syscall &S : SCV)
    addSW1Case(S);

  // Second Switch:
  // - switch based on syscall's number of args
  //   and make the call

  auto getSW2CaseBB = [&](size_t Val) {
    std::string SSS = BBPrefix;
    raw_string_ostream SS(SSS);
    SS << "sw2_case_" << Val;

    BasicBlock *BB = BasicBlock::Create(*Context, SS.str(), FRVSC, BBExit);
    Builder->SetInsertPoint(BB);

    // Set args
    std::vector<Value *> Args = { load(RV_A7) };
    for (size_t I = 0; I < Val; I++)
      Args.push_back(load(RV_A0 + I));

    // Make the syscall
    Value *V = Builder->CreateCall(FX86SC[Val], Args);
    store(V, RV_A0);
    Builder->CreateBr(BBExit);
    return BB;
  };

  BasicBlock *SW2Case0 = getSW2CaseBB(0);

  Builder->SetInsertPoint(BBSW2);
  SwitchInst *SW2 = Builder->CreateSwitch(load(RV_T0), SW2Case0);
  SW2->addCase(ConstantInt::get(I32, 0), SW2Case0);
  for (size_t I = 1; I < N; I++)
    SW2->addCase(ConstantInt::get(I32, I), getSW2CaseBB(I));

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

Error Translator::buildStack()
{
  std::string Bytes(StackSize, 'S');

  ArrayRef<uint8_t> ByteArray(
    reinterpret_cast<const uint8_t *>(Bytes.data()),
    StackSize);

  Constant *CDA = ConstantDataArray::get(*Context, ByteArray);

  Stack =
    new GlobalVariable(*Module, CDA->getType(), !CONSTANT,
      GlobalValue::ExternalLinkage, CDA, "Stack");

  return Error::success();
}


Expected<Value *> Translator::call(StringRef Func)
{
  // Load LibC module
  if (!LCModule) {
    if (!LIBC_BC) {
      SBTError SE;
      SE << "libc.bc file not found";
      return error(SE);
    }

    auto Res = MemoryBuffer::getFile(*LIBC_BC);
    if (!Res)
      return errorCodeToError(Res.getError());
    MemoryBufferRef Buf = **Res;

    auto ExpMod = parseBitcodeFile(Buf, *Context);
    if (!ExpMod)
      return ExpMod.takeError();
    LCModule = std::move(*ExpMod);
  }

  Function *F = Module->getFunction(Func);
  // "Import" function from LibC module if not found
  if (!F) {
    Function *LCF = LCModule->getFunction(Func);
    if (!LCF) {
      SBTError SE;
      SE << "Function not found: " << Func;
      return error(SE);
    }

    F = Function::Create(LCF->getFunctionType(),
          Function::ExternalLinkage, LCF->getName(), Module);
  }

  const auto &ArgList = F->getArgumentList();

  std::vector<Value *> Args;
  assert(F->arg_size() < 9 &&
      "External functions with more than 8 arguments are not supported!");

  unsigned Reg = RV_A0;
  for (const auto &Arg : ArgList) {
    Value *V = load(Reg++);
    Type *Ty = Arg.getType();

    // need to cast?
    if (Ty != I32) {
      V = Builder->CreateBitOrPointerCast(V, Ty);
      updateFirst(V);
    }

    Args.push_back(V);
  }

  // VarArgs: passing 4 extra args for now
  if (F->isVarArg()) {
    unsigned LastReg = MIN(Reg + 3, RV_A7);
    for (; Reg <= LastReg; Reg++)
      Args.push_back(load(Reg));
  }

  Value *V;
  if (F->getReturnType()->isVoidTy())
    V = Builder->CreateCall(F, Args);
  else
    V = Builder->CreateCall(F, Args, F->getName());
  updateFirst(V);
  return V;
}


llvm::Value *Translator::handleRelocation(llvm::raw_ostream &SS)
{
  IsRel = false;

  // No more relocations exist
  if (RI == RE)
    return nullptr;

  // Check if there is a relocation for current address
  auto CurReloc = *RI;
  if (CurReloc->offset() != CurAddr)
    return nullptr;

  ConstSymbolPtr Sym = CurReloc->symbol();
  llvm::StringRef SymbolName = Sym->name();
  uint64_t Type = CurReloc->type();
  ConstSectionPtr Sec = Sym->section();
  bool IsLO = false;
  uint64_t Rel = Sym->address();
  uint64_t Mask;

  // Add Section Offset (if any)
  if (Sec)
    Rel += Sec->shadowOffs();

  switch (Type) {
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_HI20:
      break;

    // This rellocation has PC info only
    // Symbol info is present on the PCREL_HI20 Reloc
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
      IsLO = true;
      Rel = (**RLast).symbol()->address();
      Rel += (**RLast).section()->shadowOffs();
      break;

    case llvm::ELF::R_RISCV_LO12_I:
      IsLO = true;
      break;

    default:
      DBGS << "Relocation Type: " << Type << '\n';
      llvm_unreachable("unknown relocation");
  }

  // Increment relocation iterator
  do {
    ++RI;
  } while (RI != RE && (**RI).offset() == CurAddr);

  // Write relocation string
  if (IsLO) {
    Mask = 0xFFF;
    SS << "%lo(";
  } else {
    Mask = 0xFFFFF000;
    SS << "%hi(";
  }
  SS << SymbolName << ") = " << Rel;

  // Now add the relocation offset to ShadowImage to get the final address

  // Get char * to memory
  std::vector<llvm::Value *> Idx = { ZERO, llvm::ConstantInt::get(I32, Rel) };
  llvm::Value *V =
    Builder->CreateGEP(ShadowImage, Idx);
  updateFirst(V);

  V = i8PtrToI32(V);

  // Finally, get only the upper or lower part of the result
  V = Builder->CreateAnd(V, llvm::ConstantInt::get(I32, Mask));
  updateFirst(V);

  IsRel = true;
  RelSym = SymbolName;
  return V;
}


Error Translator::translateLoad(
  const llvm::MCInst &Inst,
  IntType IT,
  llvm::raw_string_ostream &SS)
{
  switch (IT) {
    case U8:
      SS << "lbu";
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
  Value *Imm = getImm(Inst, 2, SS);

  Value *V = add(RS1, Imm);

  Value *Ptr;
  switch (IT) {
    case U8:
      Ptr = Builder->CreateCast(
        llvm::Instruction::CastOps::IntToPtr, V, I8Ptr);
      break;

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
    case U8:
    case U16:
      V = Builder->CreateZExt(V, I32);
      updateFirst(V);
      break;

    case U32:
      break;
  }
  store(V, O);

  setRelInfo(O);
  dbgprint(SS);
  return Error::success();
}


llvm::Error Translator::translateBranch(
  const llvm::MCInst &Inst,
  BranchType BT,
  llvm::raw_string_ostream &SS)
{
  // Inst print
  switch (BT) {
    case BEQ:
      SS << "beq";
      break;
  }
  SS << '\t';

  // get ops
  Value *RS1 = getReg(Inst, 0, SS);
  Value *RS2 = getReg(Inst, 1, SS);
  Value *Imm = getImm(Inst, 2, SS);

  // get target

  // << 1
  // Imm = Builder->CreateShl(Imm, ConstantInt::get(I32, 1));
  // updateFirst(Imm);

  // + PC
  int64_t BV = cast<ConstantInt>(Imm)->getSExtValue();
  uint64_t Target;
  if (BV >= 0) {
    Target = static_cast<uint64_t>(BV) + CurAddr;
  } else {
    SBTError SE;
    SE << "TODO Support negative branches";
    return error(SE);
  }

  // Get current function
  const Function *CF = Builder->GetInsertBlock()->getParent();
  Function *F = Module->getFunction(CF->getName());

  // Next BB
  BasicBlock *Next = BasicBlock::Create(*Context, getBBName(CurAddr + 4), F);

  // Target BB
  BasicBlock *BeforeBB = nullptr;
  typedef decltype(BBS) BBSType;
  typedef BBSType::Item Item;
  auto Iter = std::lower_bound(BBS.begin(), BBS.end(), Target,
      [](const Item& a, uint64_t b){ return a.IKey < b; });
  if (Iter != BBS.end())
    BeforeBB = Iter->IVal;

  BasicBlock *BB = BasicBlock::Create(*Context, getBBName(Target), F, BeforeBB);
  BBS(Target, std::move(BB));

  if (Target < NextBB || NextBB < CurAddr)
    NextBB = Target;

  // Evaluate condition
  Value *Cond;
  switch (BT) {
    case BEQ:
      Cond = Builder->CreateICmpEQ(RS1, RS2);
      break;
  }
  updateFirst(Cond);

  // Branch
  Value *V = Builder->CreateCondBr(Cond, BB, Next);
  updateFirst(V);

  // Use next BB
  Builder->SetInsertPoint(Next);

  dbgprint(SS);
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

} // sbt
