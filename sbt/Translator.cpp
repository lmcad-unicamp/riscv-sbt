#include "Translator.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Type.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/MemoryBuffer.h>

// LLVM internal instruction info
#define GET_INSTRINFO_ENUM
#include <lib/Target/RISCVMaster/RISCVMasterGenInstrInfo.inc>
#define GET_REGINFO_ENUM
#include <lib/Target/RISCVMaster/RISCVMasterGenRegisterInfo.inc>

#include <algorithm>
#include <vector>

#undef NDEBUG
#include <cassert>

using namespace llvm;

namespace sbt {

Translator::Translator(
  LLVMContext *Ctx,
  IRBuilder<> *Bldr,
  llvm::Module *Mod)
  :
  I32(Type::getInt32Ty(*Ctx)),
  I16(Type::getInt16Ty(*Ctx)),
  I8(Type::getInt8Ty(*Ctx)),
  I32Ptr(Type::getInt32PtrTy(*Ctx)),
  I16Ptr(Type::getInt16PtrTy(*Ctx)),
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
    BasicBlock **BB = BBMap[CurAddr];
    assert(BB && "BasicBlock not found!");
    if (!BrWasLast)
      Builder->CreateBr(*BB);
    Builder->SetInsertPoint(*BB);

    auto Iter = BBMap.lower_bound(CurAddr + 4);
    if (Iter != BBMap.end())
      updateNextBB(Iter->IKey);
  }

  Error E = noError();
  BrWasLast = false;

  switch (Inst.getOpcode())
  {
    // ALU Ops
    case RISCV::ADD:
      E = translateALUOp(Inst, ADD, false, SS);
      break;
    case RISCV::ADDI:
      E = translateALUOp(Inst, ADD, true, SS);
      break;
    case RISCV::OR:
      E = translateALUOp(Inst, OR, false, SS);
      break;
    case RISCV::ORI:
      E = translateALUOp(Inst, OR, true, SS);
      break;
    case RISCV::MUL:
      E = translateALUOp(Inst, MUL, false, SS);
      break;
    case RISCV::SLLI:
      E = translateALUOp(Inst, SLL, true, SS);
      break;

    case RISCV::AUIPC: {
      SS << "auipc\t";

      unsigned O = getRD(Inst, SS);
      auto ExpV = getImm(Inst, 1, SS);
      if (!ExpV)
        return ExpV.takeError();
      Value *V = ExpV.get();

      // Add PC (CurAddr) if V is not a relocation
      if (!LastImm.IsSym) {
        V = add(V, ConstantInt::get(I32, CurAddr));
        // Get upper 20 bits only
        V = Builder->CreateAnd(V, ConstantInt::get(I32, 0xFFFFF000));
        updateFirst(V);
      }
      // Note: for relocations, the mask was already applied to the result

      store(V, O);
      break;
    }

    // Branch
    case RISCV::BEQ:
      E = translateBranch(Inst, BEQ, SS);
      break;
    case RISCV::BGE:
      E = translateBranch(Inst, BGE, SS);
      break;
    case RISCV::BLT:
      E = translateBranch(Inst, BLT, SS);
      break;
    case RISCV::BLTU:
      E = translateBranch(Inst, BLTU, SS);
      break;

    case RISCV::ECALL: {
      SS << "ecall";

      if (Error E = handleSyscall())
        return E;
      break;
    }

    // Jump
    case RISCV::JAL:
      E = translateBranch(Inst, JAL, SS);
      break;
    case RISCV::JALR:
      E = translateBranch(Inst, JALR, SS);
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

    case RISCV::LUI: {
      SS << "lui\t";

      unsigned O = getRD(Inst, SS);
      auto ExpImm = getImm(Inst, 1, SS);
      if (!ExpImm)
        return ExpImm.takeError();
      Value *Imm = ExpImm.get();

      Value *V;
      if (LastImm.IsSym)
        V = Imm;
      else {
        V = Builder->CreateShl(Imm, ConstantInt::get(I32, 12));
        updateFirst(V);
      }
      store(V, O);

      break;
    }

    case RISCV::SB:
      E = translateStore(Inst, U8, SS);
      break;
    case RISCV::SH:
      E = translateStore(Inst, U16, SS);
      break;
    case RISCV::SW:
      E = translateStore(Inst, U32, SS);
      break;

    default: {
      SE << "Unknown instruction opcode: " << Inst.getOpcode();
      return error(SE);
    }
  }

  if (E)
    return E;

  dbgprint(SS);
  assert(First && "No First Instruction!");
  InstrMap(CurAddr, std::move(First));
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
  BBMap(Addr, std::move(BB));
  Builder->SetInsertPoint(BB);

  // Set stack pointer.

  std::vector<Value *> Idx = { ZERO, ConstantInt::get(I32, StackSize) };
  Value *V =
    Builder->CreateGEP(Stack, Idx);
  StackEnd = i8PtrToI32(V);

  store(StackEnd, RV_SP);

  // if (auto E = startup())
  //  return E;

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
  BBMap(Addr, std::move(BB));
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
    if (!Sec->isText() && !Sec->isData() && !Sec->isBSS() && !Sec->isCommon())
      continue;

    StringRef Bytes;
    std::string Z;
    // .bss/.common
    if (Sec->isBSS() || Sec->isCommon()) {
      Z = std::string(Sec->size(), 0);
      Bytes = Z;
    // others
    } else {
      // Read contents
      std::error_code EC = Sec->contents(Bytes);
      if (EC) {
        SE  << __FUNCTION__ << ": failed to get section ["
            << Sec->name() << "] contents";
        return error(SE);
      }
    }

    // Align all sections
    while (Vec.size() % 4 != 0)
      Vec.push_back(0);

    // Set Shadow Offset of Section
    Sec->shadowOffs(Vec.size());

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


llvm::Expected<llvm::Value *>
Translator::handleRelocation(llvm::raw_ostream &SS)
{
  LastImm.IsSym = false;

  // No more relocations exist
  if (RI == RE)
    return nullptr;

  // Check if there is a relocation for current address
  auto CurReloc = *RI;
  if (CurReloc->offset() != CurAddr)
    return nullptr;

  ConstSymbolPtr Sym = CurReloc->symbol();
  uint64_t Type = CurReloc->type();
  bool IsLO = false;
  uint64_t Mask;
  ConstSymbolPtr RealSym = Sym;

  switch (Type) {
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_HI20:
      break;

    // This rellocation has PC info only
    // Symbol info is present on the PCREL_HI20 Reloc
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
      IsLO = true;
      RealSym = (**RLast).symbol();
      break;

    case llvm::ELF::R_RISCV_LO12_I:
      IsLO = true;
      break;

    default:
      DBGS << "Relocation Type: " << Type << '\n';
      llvm_unreachable("unknown relocation");
  }

  // Set Symbol Relocation info
  SymbolReloc SR;
  SR.IsValid = true;
  SR.Name = RealSym->name();
  SR.Addr = RealSym->address();
  SR.Val = SR.Addr;
  SR.Sec = RealSym->section();

  // Note: !SR.Sec && SR.Addr == External Symbol
  assert((SR.Sec || !SR.Addr) && "No section found for relocation");
  if (SR.Sec) {
    assert(SR.Addr < SR.Sec->size() && "Out of bounds relocation");
    SR.Val += SR.Sec->shadowOffs();
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
  SS << SR.Name << ") = " << SR.Addr;

  Value *V = nullptr;
  // special case: external functions: return our handler instead
  if (SR.isExternal()) {
    auto ExpF = import(SR.Name);
    if (!ExpF)
      return ExpF.takeError();
    Function *F = ExpF.get();
    V = Module->getValueSymbolTable().lookup(F->getName());
    assert(V && "External function handler not found");
    V = Builder->CreatePtrToInt(V, I32);
    updateFirst(V);
    V = Builder->CreateAnd(V, llvm::ConstantInt::get(I32, Mask));
    updateFirst(V);

  // add the relocation offset to ShadowImage to get the final address
  } else if (SR.Sec) {
    // get char * to memory
    std::vector<llvm::Value *> Idx = { ZERO,
      llvm::ConstantInt::get(I32, SR.Val) };
    V = Builder->CreateGEP(ShadowImage, Idx);
    updateFirst(V);

    V = i8PtrToI32(V);

    // Finally, get only the upper or lower part of the result
    V = Builder->CreateAnd(V, llvm::ConstantInt::get(I32, Mask));
    updateFirst(V);

  } else
    llvm_unreachable("Failed to resolve relocation");

  LastImm.IsSym = true;
  LastImm.SymRel = std::move(SR);
  return V;
}


llvm::Error Translator::translateALUOp(
  const llvm::MCInst &Inst,
  ALUOp Op,
  bool HasImm,
  llvm::raw_string_ostream &SS)
{
  switch (Op) {
    case ADD: SS << "add"; break;
    case OR:  SS << "or"; break;
    case MUL: SS << "mul"; break;
    case SLL: SS << "sll"; break;
  }
  if (HasImm)
    SS << "i";
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

    case OR:
      V = Builder->CreateOr(O1, O2);
      break;

    case MUL:
      V = Builder->CreateMul(O1, O2);
      break;

    case SLL:
      V = Builder->CreateShl(O1, O2);
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
  BrWasLast = true;

  // Inst print
  switch (BT) {
    case JAL:   SS << "jal";  break;
    case JALR:  SS << "jalr"; break;
    case BEQ:   SS << "beq";  break;
    case BGE:   SS << "bge";  break;
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
      O1N = getRegNum(1, Inst, SS);
      O1 = getReg(Inst, 1, SS);
      if (!exp(getImm(Inst, 2, SS), O2, E))
        return E;
      JReg = O1;
      JImm = O2;
      break;

    case BEQ:
    case BGE:
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
    IJUMP
  };
  Action Act;

  // JALR
  //
  // TODO Set Target LSB to zero
  if (BT == JALR) {
    // No base reg
    if (O1N == RV_ZERO) {
      assert(IsSymbol && "Unexpected JALR with absolute immediate");
      SR = LastImm.SymRel;
      Act = JUMP_TO_SYMBOL;

    // No immediate
    } else if (JImmIsZeroImm) {
      V = JReg;
      Act = IJUMP;

    // Base + Offset
    } else {
      V = Builder->CreateAdd(JReg, JImm);
      updateFirst(V);
      Act = IJUMP;
    }

  // JAL to Symbol
  } else if (BT == JAL && IsSymbol) {
    SR = LastImm.SymRel;
    Act = JUMP_TO_SYMBOL;

  // JAL/Branches to PC offsets
  //
  // Add PC
  } else {
    Target = JImmI + CurAddr;
    Act = JUMP_TO_OFFS;
  }

  if (Act == JUMP_TO_SYMBOL) {
    if (SR.isExternal()) {
      V = JImm;
      Act = IJUMP;
    } else {
      assert(SR.Sec && SR.Sec->isText() && "Jump to non Text Section!");
      Target = SR.Addr;
      Act = JUMP_TO_OFFS;
    }
  }

  // Evaluate condition
  Value *Cond = nullptr;
  switch (BT) {
    case BEQ:
      Cond = Builder->CreateICmpEQ(O0, O1);
      break;

    case BGE:
      Cond = Builder->CreateICmpSGE(O0, O1);
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
      break;

    case JUMP_TO_OFFS:
      if (auto E = handleJumpToOffs(Target, Cond, LinkReg))
        return E;
      break;

    case IJUMP:
      if (auto E = handleIJump(V, LinkReg))
        return E;
      break;
  }

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
  if (!Cond && LinkReg != RV_ZERO)
    store(ConstantInt::get(I32, NextInstrAddr), LinkReg);

  // Target BB
  assert(Target != CurAddr && "Unexpected jump to self instruction!");

  BasicBlock *BB = nullptr;
  auto Iter = BBMap.lower_bound(Target);
  // Jump forward
  if (Target > CurAddr) {
    BasicBlock *BeforeBB = nullptr;
    if (Iter != BBMap.end())
      BeforeBB = Iter->IVal;

    // BB already exists
    if (Target == Iter->IKey)
      BB = Iter->IVal;
    // Need to create new BB
    else {
      BB = BasicBlock::Create(*Context, getBBName(Target), F, BeforeBB);
      BBMap(Target, std::move(BB));

      updateNextBB(Target);
    }

  // Jump backward
  } else {
    bool Split = true;
    if (Iter != BBMap.end()) {
      if (Target == Iter->IKey) {
        Split = false;
        BB = Iter->IVal;
      } else if (Iter != BBMap.begin()) {
        BB = (--Iter)->IVal;
      } else {
        BB = Iter->IVal;
      }
    } else {
      assert(BBMap.begin() != BBMap.end());
      BB = (--Iter)->IVal;
    }

    if (Split)
      BB = splitBB(BB, Target);
  }

  // Branch
  Value *V;
  if (Cond && Target != NextInstrAddr) {
    Iter = BBMap.lower_bound(NextInstrAddr);
    BasicBlock *BeforeBB = nullptr;
    if (Iter != BBMap.end())
      BeforeBB = Iter->IVal;

    BasicBlock *Next =
      BasicBlock::Create(*Context, getBBName(NextInstrAddr), F, BeforeBB);
    BBMap(NextInstrAddr, std::move(Next));

    updateNextBB(NextInstrAddr);

    V = Builder->CreateCondBr(Cond, BB, Next);
    // Use next BB
    Builder->SetInsertPoint(Next);
  } else
    V = Builder->CreateBr(BB);
  updateFirst(V);

  return Error::success();
}


llvm::BasicBlock *Translator::splitBB(
  llvm::BasicBlock *BB,
  uint64_t Addr)
{
  auto Res = InstrMap[Addr];
  assert(Res && "Instruction not found!");

  Instruction *TgtInstr = *Res;

  BasicBlock::iterator I, E;
  for (I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (&*I == TgtInstr)
      break;
  }
  assert(I != E);

  BasicBlock *BB2;
  if (BB->getTerminator()) {
    BB2 = BB->splitBasicBlock(I, getBBName(Addr));
    BBMap(Addr, std::move(BB2));
    return BB2;
  }

  // Insert dummy terminator
  assert(Builder->GetInsertBlock() == BB);
  Instruction *Instr = Builder->CreateRetVoid();
  BB2 = BB->splitBasicBlock(I, getBBName(Addr));
  BBMap(Addr, std::move(BB2));
  Instr->eraseFromParent();
  Builder->SetInsertPoint(BB2, BB2->end());

  return BB2;
}

llvm::Error Translator::handleIJump(
  llvm::Value *Target,
  unsigned LinkReg)
{
  assert(LinkReg == RV_RA);

  /*
  // create BB for next instruction
  uint64_t NextInstrAddr = CurAddr + 4;
  BasicBlock *CurBB = Builder->GetInsertBlock();

  BasicBlock *TargetBB = BasicBlock::Create(
    *Context, getBBName(NextInstrAddr), CurBB->getParent());
  BBMap(NextInstrAddr, std::move(TargetBB));

  // link
  store(ConstantInt::get(I32, NextInstrAddr), LinkReg);
  */

  // indirect call
  FunctionType *FT = FunctionType::get(Builder->getVoidTy(), !VAR_ARG);
  Value *V = Builder->CreateIntToPtr(Target, FT->getPointerTo());
  updateFirst(V);
  V = Builder->CreateCall(V);
  updateFirst(V);

  return Error::success();
}

llvm::Error Translator::startup()
{
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), !VAR_ARG);
  Function *F =
    Function::Create(FT, Function::PrivateLinkage, "rv32_startup", Module);

  // BB
  BasicBlock *BB = BasicBlock::Create(*Context, "bb_startup", F);
  BasicBlock *MainBB = Builder->GetInsertBlock();

  Builder->CreateCall(F);
  Builder->SetInsertPoint(BB);
  Builder->CreateRetVoid();
  Builder->SetInsertPoint(MainBB);

  return Error::success();
}

llvm::Expected<llvm::Function *> Translator::import(llvm::StringRef Func)
{
  std::string RV32Func = "rv32_" + Func.str();

  // check if the function was already processed
  if (Function *F = Module->getFunction(RV32Func))
    return F;

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

  // lookup function
  Function *LF = LCModule->getFunction(Func);
  if (!LF) {
    SBTError SE;
    SE << "Function not found: " << Func;
    return error(SE);
  }
  FunctionType *FT = LF->getFunctionType();

  // declare imported function in our module
  Function *IF =
    Function::Create(FT, GlobalValue::ExternalLinkage, Func, Module);

  // create our caller to the external function
  FunctionType *VFT = FunctionType::get(Builder->getVoidTy(), !VAR_ARG);
  Function *F =
    Function::Create(VFT, GlobalValue::PrivateLinkage, RV32Func, Module);

  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  BasicBlock *PrevBB = Builder->GetInsertBlock();
  Builder->SetInsertPoint(BB);

  OnScopeExit RestoreInsertPoint(
    [this, PrevBB]() {
      Builder->SetInsertPoint(PrevBB);
    });

  unsigned NumParams = FT->getNumParams();
  assert(NumParams < 9 &&
      "External functions with more than 8 arguments are not supported!");

  // build Args
  std::vector<Value *> Args;
  unsigned Reg = RV_A0;
  unsigned I = 0;
  for (; I < NumParams; I++) {
    Value *V = load(Reg++);
    Type *Ty = FT->getParamType(I);

    // need to cast?
    if (Ty != I32)
      V = Builder->CreateBitOrPointerCast(V, Ty);

    Args.push_back(V);
  }

  // VarArgs: passing 4 extra args for now
  if (FT->isVarArg()) {
    unsigned N = MIN(I + 4, 8);
    for (; I < N; I++)
      Args.push_back(load(Reg++));
  }

  // call the Function
  Value *V;
  if (FT->getReturnType()->isVoidTy()) {
    V = Builder->CreateCall(IF, Args);
    updateFirst(V);
  } else {
    V = Builder->CreateCall(IF, Args, IF->getName());
    updateFirst(V);
    store(V, RV_A0);
  }

  V = Builder->CreateRetVoid();
  updateFirst(V);
  return F;
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
