#include "Translator.h"

#include "Constants.h"
#include "SBTError.h"
#include "Utils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/MC/MCInst.h>
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
static unsigned RVReg(unsigned Reg)
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

      // RD
      const MCOperand &ORD = Inst.getOperand(0);
      unsigned NRD = RVReg(ORD.getReg());
      DBGS << "X" << NRD << ", ";

      // RS1
      const MCOperand &ORS1 = Inst.getOperand(1);
      unsigned NRS1 = RVReg(ORS1.getReg());
      Value *VRS1;
      if (NRS1 == 0)
        VRS1 = ZERO;
      else
        VRS1 = load(NRS1);
      DBGS << "X" << NRS1 << ", ";

      // RS2/Imm
      const MCOperand &ORS2 = Inst.getOperand(2);
      Value *VRS2;
      if (ORS2.isReg()) {
        unsigned NRS2 = RVReg(ORS2.getReg());
        if (NRS2 == 0)
          VRS2 = ZERO;
        else
          VRS2 = load(NRS2);
        DBGS << "X" << NRS2;
      } else if (ORS2.isImm()) {
        int64_t Imm = ORS2.getImm();
        VRS2 = ConstantInt::get(I32, Imm);
        DBGS << Imm;
      } else {
        SE << "RS2 is neither Reg nor Imm\n";
        return error(SE);
      }

      // ADD
      Value *V = Builder->CreateAdd(VRS1, VRS2);
      store(V, NRD);

      DBGS << "\n";
      break;
    }

    case RISCV::AUIPC: {
      DBGS << "AUIPC ";

      const MCOperand &ORD = Inst.getOperand(0);
      unsigned NRD = RVReg(ORD.getReg());
      DBGS << "X" << NRD << ", ";

      /*
      const MCOperand &OImm = Inst.getOperand(1);
      int64_t Imm = OImm.getImm();
      DBGS << Imm << "\n";
      Value *VImm = ConstantInt::get(I32, Imm);

      auto ExpR = resolveRelocation();
      if (!ExpR)
        return ExpR.takeError();
      Value *V = Builder->CreateAdd(ExpR.get(), VImm);
      store(V, NRD);
      */
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
  // FunctionAddrs.push_back(Addr);

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
    std::string RegName = Twine("X").concat(Twine(I)).str();
    X[I] = new GlobalVariable(*Module, I32, !CONSTANT,
        GlobalValue::ExternalLinkage, X0, RegName);
  }
}

/*
Expected<Value *> Translator::resolveRelocation()
{
  SBTError SE;

  // debug
  DBGS << "Section: " << CurSection->name() << "\n";
  DBGS << "CurObj: " << CurObj->fileName() << "\n";

  Value *V = nullptr;

  // find relocation section
  bool Found = false;
  object::SectionRef RelocSec;
  for (const Section &Sec : CurObj->sections()) {
    StringRef Name;
    std::error_code EC = Section.getName(Name);
    if (EC) {
      SE << "Failed to get section name";
      return error(SE);
    }

    DBGS << "Name: " << Name << "\n";
    Iter = Section.getRelocatedSection();
    if (Iter != End) {
      // RelocSec = *Iter;
      Found = true;
      DBGS << "Reloc stuff found\n";
      break;
    }
  }

  if (!Found) {
    SE << "Failed to get relocated section";
    return error(SE);
  }

  // find relocations
  StringRef Name;
  object::relocation_iterator Rel = CurSection->relocation_end();
  iterator_range<object::relocation_iterator> range =
    Iter->relocations();
  // object::relocation_iterator RI = range.begin(),
  //  RE = range.end();
  {
    uint64_t offset = getELFOffset(*CurSection);
    uint64_t addr = 0;
    for (auto RI : range) {
      outs() << "Hello\n";
      outs().flush();
      object::SymbolRef Symbol = *RI.getSymbol();
      auto Exp = Symbol.getName();
      if (!Exp) {
        SE << "Relocated symbol name not found";
        return error(SE);
      } else {
        Name = Exp.get();
        outs() << "Found relocation for: " << Name << "\n";
      }

      addr = RI.getOffset();
      if (addr + offset == CurAddr)
        break;

      Rel = RI;
    }
  }

  //if (Rel == RE) {
    SE << "Aborted";
    return error(SE);
  //}
  // uint64_t Type = Rel->getType();

  uint64_t Reloc = 0;
  // Check if relocation is a section
  for (const object::SectionRef &Section : CurObj->sections()) {
    StringRef SectionName;
    Section.getName(SectionName);
    if (SectionName != Name)
      continue;

    uint64_t SectionAddr = Section.getAddress();
    if (SectionAddr == 0)
      SectionAddr = getELFOffset(Section);
    Reloc = SectionAddr;
    outs() << "Relocation is a section\n";
  }

  if (!Reloc) {
    // Check if relocation is a symbol
    for (const object::SymbolRef &Symbol : CurObj->symbols()) {
      auto Exp = Symbol.getName();
      if (!Exp) {
        SE << "Failed to get symbol name";
        return error(SE);
      }
      const StringRef &SymbolName = Exp.get();
      if (SymbolName != Name)
        continue;

      auto ExpAddr = Symbol.getAddress();
      if (!ExpAddr) {
        SE << "Failed to get symbol address";
        return error(SE);
      }
      uint64_t Addr = ExpAddr.get();
      Reloc = Addr;

      // Check if it is relative to a section.
      auto ExpSection = Symbol.getSection();
      if (!ExpSection) {
        SE << "Failed to get section from symbol";
        return error(SE);
      }
      object::section_iterator Iter = ExpSection.get();
      if (Iter != CurObj->section_end()) {
        StringRef SectionName;
        std::error_code EC = Iter->getName(SectionName);
        if (!EC && SectionName == ".text") {
          // symbol not found
        }

        Addr = Iter->getAddress();
        if (Addr == 0)
          Addr = getELFOffset(*Iter);
        Reloc += Addr;
      }
      outs() << "Relocation is a symbol\n";
      break;
    }

    if (!Reloc) {
      SE << "Relocation not found(2)\n";
      return error(SE);
    }
  }

  V = ConstantInt::get(I32, Reloc);
  return V;
}
*/

} // sbt
