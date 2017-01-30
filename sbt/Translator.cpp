#include "Translator.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FormatVariadic.h>

#define GET_INSTRINFO_ENUM
#include "../build-llvm/lib/Target/RISCVMaster/RISCVMasterGenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "../build-llvm/lib/Target/RISCVMaster/RISCVMasterGenRegisterInfo.inc"

using namespace llvm;

namespace sbt {

Translator::Translator(
  LLVMContext *Ctx,
  IRBuilder<> *Bldr,
  llvm::Module *Mod)
  :
  I32(Type::getInt32Ty(*Ctx)),
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
  Instruction *First = nullptr;

  // Add RV Inst metadata and print it in debug mode
#if SBT_DEBUG
  auto MDName = [](const llvm::StringRef &S) {
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
    SS.flush();
    return SSS;
  };

# define DBGPRINT(s) \
  do { \
    SS.flush(); \
    DBGS << SSS << "\n"; \
    MDNode *N = MDNode::get(*Context, MDString::get(*Context, "RISC-V Instruction")); \
    First->setMetadata(MDName(SSS), N); \
  } while (0)

  std::string SSS;
  raw_string_ostream SS(SSS);
#else
# define DBGPRINT(s)  do { ; } while(0)
  raw_ostream &SS = nulls();
#endif
  SS << formatv("{0:X-4}:   ", CurAddr);

  switch (Inst.getOpcode())
  {
    case RISCV::ADDI: {
      SS << "addi\t";

      unsigned O = getRD(Inst, SS);
      Value *O1 = getReg(Inst, 1, First, SS);
      Value *O2 = getRegOrImm(Inst, 2, First, SS);

      // If this is a relocation, get lower 12 bits only
      // TODO Review this
      if (!isa<ConstantInt>(O2)) {
        O2 = Builder->CreateAnd(O2, ConstantInt::get(I32, 0xFFF));
        updateFirst(O2, First);
      }

      Value *V = add(O1, O2, First);
      store(V, O, First);

      DBGPRINT(SS);
      break;
    }

    case RISCV::AUIPC: {
      SS << "auipc\t";

      unsigned O = getRD(Inst, SS);
      Value *V = getImm(Inst, 1, First, SS);

      // Add PC (CurAddr) if V is not a relocation
      // TODO Review this
      if (isa<ConstantInt>(V))
        V = add(V, ConstantInt::get(I32, CurAddr), First);

      // Get upper 20 bits only
      V = Builder->CreateAnd(V, ConstantInt::get(I32, 0xFFFFF000));
      updateFirst(V, First);

      store(V, O, First);

      DBGPRINT(SS);
      break;
    }

    case RISCV::ECALL: {
      SS << "ecall";

      if (Error E = handleSyscall(First))
        return E;

      DBGPRINT(SS);
      break;
    }

    default: {
      SE << "Unknown instruction opcode: "
         << Inst.getOpcode() << "\n";
      return error(SE);
    }
  }

  return Error::success();
}

Error Translator::startModule()
{
  // BasicBlock *BB = Builder->GetInsertBlock();

  buildRegisterFile();

  if (Error E = buildShadowImage())
    return E;

  if (Error E = genSyscallHandler())
    return E;

  return Error::success();
}

Error Translator::finishModule()
{
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
  Twine BBName = Twine("bb").concat(Twine::utohexstr(Addr));
  BasicBlock *BB = BasicBlock::Create(*Context, BBName, F);
  Builder->SetInsertPoint(BB);

  return Error::success();
}

Error Translator::finishFunction()
{
  Builder->CreateRetVoid();
  return Error::success();
}

void Translator::buildRegisterFile()
{
  Constant *X0 = ConstantInt::get(I32, 0u);
  X[0] = new GlobalVariable(*Module, I32, CONSTANT,
    GlobalValue::ExternalLinkage, X0, IREGNAME + "0");

  for (int I = 1; I < 32; ++I) {
    std::string S;
    raw_string_ostream SS(S);
    SS << IREGNAME << I;
    X[I] = new GlobalVariable(*Module, I32, !CONSTANT,
        GlobalValue::ExternalLinkage, X0, SS.str());
  }
}

Error Translator::buildShadowImage()
{
  SBTError SE;

  // Assuming only one data section for now
  // Get Data Section
  ConstSectionPtr DS = nullptr;
  for (ConstSectionPtr Sec : CurObj->sections()) {
    if (Sec->section().isData() && !Sec->isText()) {
      // DBGS << __FUNCTION__ << ": " << Sec->name() << "\n";
      DS = Sec;
      break;
    }
  }

  if (!DS) {
    SE << __FUNCTION__ << ": No Data Section found";
    return error(SE);
  }

  StringRef Bytes;
  std::error_code EC = DS->contents(Bytes);
  if (EC) {
    SE << __FUNCTION__ << ": failed to get section contents";
    return error(SE);
  }

  ArrayRef<uint8_t> ByteArray(
    reinterpret_cast<const uint8_t *>(Bytes.data()),
    Bytes.size());

  Constant *CDA = ConstantDataArray::get(*Context, ByteArray);

  ShadowImage =
    new GlobalVariable(*Module, CDA->getType(), !CONSTANT,
      GlobalValue::ExternalLinkage, CDA, "ShadowMemory");

  return Error::success();
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
    { 4, 64, 4 }   // WRITE
  };

  const std::string BBPrefix = "bb_rvsc_";
  llvm::Instruction *First = nullptr;

  // Define rv_syscall function
  FTRVSC = FunctionType::get(I32, { I32 }, !VAR_ARG);
  FRVSC =
    Function::Create(FTRVSC, Function::ExternalLinkage, "rv_syscall", Module);

  // Entry
  BasicBlock *BBEntry = BasicBlock::Create(*Context, BBPrefix + "entry", FRVSC);
  llvm::Argument &SC = *FRVSC->arg_begin();

  // Exit
  //
  // Return A0
  BasicBlock *BBExit = BasicBlock::Create(*Context,
    BBPrefix + "exit", FRVSC);
  Builder->SetInsertPoint(BBExit);
  Builder->CreateRet(load(RV_A0, First));

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
  store(ConstantInt::get(I32, 1), RV_T0, First);
  store(ConstantInt::get(I32, X86_SYS_EXIT), RV_A7, First);
  store(ConstantInt::get(I32, 99), RV_A0, First);
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
    store(ConstantInt::get(I32, S.Args), RV_T0, First);
    store(ConstantInt::get(I32, S.X86), RV_A7, First);
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
    std::vector<Value *> Args = { &SC };
    for (size_t I = 0; I < Val; I++)
      Args.push_back(load(RV_A0 + I, First));

    // Make the syscall
    Value *V = Builder->CreateCall(FX86SC[Val], Args);
    store(V, RV_A0, First);
    Builder->CreateBr(BBExit);
    return BB;
  };

  BasicBlock *SW2Case0 = getSW2CaseBB(0);

  Builder->SetInsertPoint(BBSW2);
  SwitchInst *SW2 = Builder->CreateSwitch(load(RV_T0, First), SW2Case0);
  SW2->addCase(ConstantInt::get(I32, 0), SW2Case0);
  for (size_t I = 1; I < N; I++)
    SW2->addCase(ConstantInt::get(I32, I), getSW2CaseBB(I));

  return Error::success();
}

Error Translator::handleSyscall(llvm::Instruction *&First)
{
  Value *SC = load(RV_A7, First);
  std::vector<Value *> Args = { SC };
  Value *V = Builder->CreateCall(FRVSC, Args);
  updateFirst(V, First);
  store(V, RV_A0, First);

  return Error::success();
}

} // sbt
