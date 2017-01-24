#include "Translator.h"

#include "Constants.h"
#include "SBTError.h"
#include "Utils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Object/ObjectFile.h>

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
  Module(Mod),
  FTSyscall4(FunctionType::get(I32,
    { I32, I32, I32, I32, I32 }, !VAR_ARG)),
  FSyscall4(Function::Create(FTSyscall4,
    Function::ExternalLinkage, "syscall", Module))
{
  buildRegisterFile();
}

Translator::~Translator()
{
  // Destroy register file
  for (int i = 0; i < 32; i++)
    delete X[i];
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

Value *Translator::load(unsigned Reg)
{
  Value *V = Builder->CreateLoad(X[Reg], X[Reg]->getName());
  return V;
}

void Translator::store(Value *V, unsigned Reg)
{
  Builder->CreateStore(V, X[Reg], !VOLATILE);
}

Error Translator::translate(const llvm::MCInst &Inst)
{
  namespace RISCV = RISCVMaster;

  SBTError SE;

  switch (Inst.getOpcode())
  {
    case RISCV::ADDI: {
      DBGS << "ADDI ";

      unsigned O = getRD(Inst);
      Value *O1 = getReg(Inst, 1);
      Value *O2 = getRegOrImm(Inst, 2);

      Value *V = Builder->CreateAdd(O1, O2);
      store(V, O);

      DBGS << "\n";
      break;
    }

    case RISCV::AUIPC: {
      DBGS << "AUIPC ";

      unsigned O = getRD(Inst);
      int64_t I = getImm(Inst, 1);
      uint64_t R = handleRelocation();
      int64_t PC = CurAddr;
      int64_t S;
      if (R)  // relocation
        S = R;
      else
        S = PC + I;

      Value *V = ConstantInt::get(I32, S & 0xFFFFF000);
      store(V, O);

      DBGS << S << "\n";
      break;
    }

    case RISCV::ECALL: {
      DBGS << "ECALL\n";

      // call
      Value *SC = load(17);
      Value *A0 = load(10);
      Value *A1 = load(11);
      Value *A2 = load(12);
      Value *A3 = load(13);
      std::vector<Value *> Args = { SC, A0, A1, A2, A3 };
      Builder->CreateCall(FSyscall4, Args);
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

Error Translator::startFunction(StringRef Name, uint64_t Addr)
{
  // Create a function with no parameters
  FunctionType *FT =
    FunctionType::get(Type::getVoidTy(*Context), !VAR_ARG);
  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, Module);

  // BB
  Twine BBName = Twine("bb").concat(Twine::utohexstr(Addr));
  BasicBlock *BB =
    BasicBlock::Create(*Context, BBName, F);
  Builder->SetInsertPoint(BB);

  if (FirstFunction) {
    FirstFunction = false;
    // CurFunAddr = Addr;
  }

  return Error::success();
}

void Translator::buildRegisterFile()
{
  Constant *X0 = ConstantInt::get(I32, 0u);
  X[0] = new GlobalVariable(*Module, I32, CONSTANT,
    GlobalValue::ExternalLinkage, X0, "X0");

  for (int I = 1; I < 32; ++I) {
    std::string S;
    raw_string_ostream SS(S);
    SS << "X" << I;
    std::string RegName = SS.str();
    X[I] = new GlobalVariable(*Module, I32, !CONSTANT,
        GlobalValue::ExternalLinkage, X0, RegName);
  }
}

} // sbt
