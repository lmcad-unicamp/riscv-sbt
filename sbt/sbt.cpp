#include "sbt.h"

#include "Constants.h"
#include "SBTError.h"
#include "Translator.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#define TEST 1
#if TEST
#include "Object.h"
#endif

using namespace llvm;

// RISCVMaster initializers
extern "C" void LLVMInitializeRISCVMasterAsmParser();
extern "C" void LLVMInitializeRISCVMasterDisassembler();
extern "C" void LLVMInitializeRISCVMasterTargetMC();
extern "C" void LLVMInitializeRISCVMasterTarget();
extern "C" void LLVMInitializeRISCVMasterTargetInfo();

namespace llvm {
Target &getTheRISCVMaster32Target();
}

// SBT

namespace sbt {

void SBT::init()
{
  initConstants();

  // Print stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(*BIN_NAME);

  // Init LLVM targets
  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();

  // Init RISCVMaster 'target'
  LLVMInitializeRISCVMasterTargetInfo();
  LLVMInitializeRISCVMasterTargetMC();
  LLVMInitializeRISCVMasterAsmParser();
  LLVMInitializeRISCVMasterDisassembler();
  // skip registration of RISCVMaster target to avoid
  // conflicts with RISCV target
  // LLVMInitializeRISCVMasterTarget();
}

void SBT::finish()
{
  llvm_shutdown();
  destroyConstants();
}

Expected<SBT> SBT::create(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile)
{
  Error Err = Error::success();
  SBT S(InputFiles, OutputFile, Err);
  if (Err)
    return std::move(Err);
  return std::move(S);
}

SBT::SBT(
  const cl::list<std::string> &InputFilesList,
  const std::string &OutputFile,
  Error &E)
  :
  OutputFile(OutputFile),
  Context(new LLVMContext),
  Builder(*Context),
  Module(new llvm::Module("main", *Context)),
  SBTTranslator(new Translator(&*Context, &Builder, &*Module))
{
  for (auto File : InputFilesList) {
    if (!sys::fs::exists(File)) {
      SBTError SE(File);
      SE << "No such file.\n";
      E = error(SE);
      return;
    }
    InputFiles.push_back(File);
  }

  SBTError SE;
  auto error = [&E](SBTError &EE) {
    E = sbt::error(std::move(EE));
  };

  // get target
  Triple Triple("riscv32-unknown-elf");
  std::string TripleName = Triple.getTriple();
  logs() << "Triple: " << TripleName << "\n";

  std::string StrError;
  Target = &getTheRISCVMaster32Target();
  // Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!Target) {
    SE << "Target not found: " << TripleName << ": " << StrError << "\n";
    error(SE);
    return;
  }

  MRI.reset(Target->createMCRegInfo(TripleName));
  if (!MRI) {
    SE << "No register info for target " << TripleName << "\n";
    error(SE);
    return;
  }

  // Set up disassembler.
  AsmInfo.reset(Target->createMCAsmInfo(*MRI, TripleName));
  if (!AsmInfo) {
    SE << "No assembly info for target " << TripleName << "\n";
    error(SE);
    return;
  }

  SubtargetFeatures Features;
  STI.reset(
      Target->createMCSubtargetInfo(TripleName, "", Features.getString()));
  if (!STI) {
    SE << "No subtarget info for target " << TripleName << "\n";
    error(SE);
    return;
  }

  MOFI.reset(new MCObjectFileInfo);
  MC.reset(new MCContext(AsmInfo.get(), MRI.get(), MOFI.get()));
  DisAsm.reset(Target->createMCDisassembler(*STI, *MC));
  if (!DisAsm) {
    SE << "No disassembler for target " << TripleName << "\n";
    error(SE);
  }

  MII.reset(Target->createMCInstrInfo());
  if (!MII) {
    SE << "No instruction info for target " << TripleName << "\n";
    error(SE);
    return;
  }

  InstPrinter.reset(
    Target->createMCInstPrinter(Triple, 0, *AsmInfo, *MII, *MRI));
  if (!InstPrinter) {
    SE << "No instruction printer for target " << TripleName << "\n";
    error(SE);
    return;
  }
}

Error SBT::run()
{
  // genHello();

  for (auto &F : InputFiles) {
    Error E = translate(F);
    if (E)
      return E;
  }
  return Error::success();
}

void SBT::dump() const
{
  Module->dump();
}

void SBT::write()
{
  std::error_code EC;
  raw_fd_ostream OS(OutputFile, EC, sys::fs::F_None);
  WriteBitcodeToFile(&*Module, OS);
  OS.flush();
}

Error SBT::translate(const std::string &File)
{
  SBTError SE(File);

  // attempt to open the binary.
  auto Exp = object::createBinary(File);
  if (!Exp) {
    SE << Exp.takeError() << "Failed to open binary file\n";
    return error(SE);
  }
  object::OwningBinary<object::Binary> &OB = Exp.get();
  object::Binary *Bin = OB.getBinary();

  // for now, only object files are supported
  if (!object::ObjectFile::classof(Bin)) {
    SE << "Unrecognized file type.\n";
    return error(SE);
  }

  auto Obj = dyn_cast<object::ObjectFile>(Bin);
  auto log = [&]() -> raw_ostream & {
    return logs() << Obj->getFileName() << ": ";
  };

  log() << "File Format: " << Obj->getFileFormatName() << "\n";
  SBTTranslator->setCurObj(Obj);

  // for each section
  for (const object::SectionRef &Section : Obj->sections()) {
    // skip non code sections
    if (!Section.isText())
      continue;

    SBTTranslator->setCurSection(&Section);

    // get section name
    StringRef SectionName;
    std::error_code EC = Section.getName(SectionName);
    if (EC) {
      SE << "Failed to get Section Name";
      return error(SE);
    }
    DBGS << "Disassembly of section " << SectionName << ":\n";

    // get section bytes
    StringRef BytesStr;
    EC = Section.getContents(BytesStr);
    if (EC) {
      SE << "Failed to get Section Contents";
      return error(SE);
    }
    ArrayRef<uint8_t> Bytes(reinterpret_cast<const uint8_t *>(BytesStr.data()),
                            BytesStr.size());

    // Make a list of all the symbols in this section.
    auto Exp = getSymbolsList(Obj, Section);
    if (!Exp) {
      SE << Exp.takeError() << "Failed to get SymbolsList";
      return error(SE);
    }
    SymbolVec &Symbols = Exp.get();
    // If the section has no symbols just insert a dummy one and disassemble
    // the whole section.
    if (Symbols.empty())
      Symbols.push_back({0, SectionName});

    uint64_t SectionAddr = Section.getAddress();
    uint64_t SectionSize = Section.getSize();

    // for each symbol
    for (unsigned SI = 0, SN = Symbols.size(); SI != SN; ++SI) {
      const auto &Symbol = Symbols[SI];
      uint64_t Start = Symbol.first;
      uint64_t End;
      const StringRef &SymbolName = Symbol.second;

      // The end is either the size of the section or the beginning of the next
      // symbol.
      if (SI == SN - 1)
        End = SectionSize;
      // Make sure this symbol takes up space.
      else if (Symbols[SI + 1].first != Start)
        End = Symbols[SI + 1].first - 1;
      else
        // This symbol has the same address as the next symbol. Skip it.
        continue;
      DBGS << SymbolName << ":\n";

      // Relocatable object
      uint64_t ELFOffset = SectionAddr;
      if (SectionAddr == 0)
        ELFOffset = getELFOffset(Section);

      if (SymbolName == "main")
        ; // TODO start main
      else {
        Error E = SBTTranslator->startFunction(SymbolName, Start + ELFOffset);
        if (E)
          return E;
      }

      // for each instruction
      uint64_t Size;
      for (uint64_t Index = Start; Index < End; Index += Size) {
        SBTTranslator->setCurAddr(Index + ELFOffset);

        MCInst Inst;
        MCDisassembler::DecodeStatus st =
          DisAsm->getInstruction(Inst, Size, Bytes.slice(Index),
            SectionAddr + Index, DBGS, nulls());
        if (st == MCDisassembler::DecodeStatus::Success) {
#ifndef NDEBUG
          InstPrinter->printInst(&Inst, DBGS, "", *STI);
          DBGS << "\n";
#endif
          Error E = SBTTranslator->translate(Inst);
          if (E)
            return E;
        } else {
          SE << "Invalid instruction encoding\n";
          return error(SE);
        }
      } // for each instruction
      // TODO finish function
    } // for each symbol
  } // for each section
  // TODO finish module

  return Error::success();
}

Error SBT::genHello()
{
  // constants
  static const int SYS_EXIT = 1;
  static const int SYS_WRITE = 4;

  // types
  Type *Int8 = Type::getInt8Ty(*Context);
  Type *Int32 = Type::getInt32Ty(*Context);

  // syscall
  FunctionType *FTSyscall =
    FunctionType::get(Int32, { Int32, Int32 }, VAR_ARG);
  Function *FSyscall = Function::Create(FTSyscall,
      Function::ExternalLinkage, "syscall", &*Module);

  // set data
  std::string Hello("Hello, World!\n");
  GlobalVariable *Msg =
    new GlobalVariable(
      *Module,
      ArrayType::get(Int8, Hello.size()),
      CONSTANT,
      GlobalVariable::PrivateLinkage,
      ConstantDataArray::getString(*Context, Hello.c_str(), !ADD_NULL),
      "msg");

  // _start
  FunctionType *FTStart = FunctionType::get(Int32, {}, !VAR_ARG);
  Function *FStart =
    Function::Create(FTStart, Function::ExternalLinkage, "_start", &*Module);

  // entry basic block
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", FStart);
  Builder.SetInsertPoint(BB);

  // call write
  Value *SC = ConstantInt::get(Int32, APInt(32, SYS_WRITE, SIGNED));
  Value *FD = ConstantInt::get(Int32, APInt(32, 1, SIGNED));
  Value *Ptr = Msg;
  Value *Len = ConstantInt::get(Int32, APInt(32, Hello.size(), SIGNED));
  std::vector<Value *> Args = { SC, FD, Ptr, Len };
  Builder.CreateCall(FSyscall, Args);

  // call exit
  SC = ConstantInt::get(Int32, APInt(32, SYS_EXIT, SIGNED));
  Value *RC = ConstantInt::get(Int32, APInt(32, 0, SIGNED));
  Args = { SC, RC };
  Builder.CreateCall(FSyscall, Args);
  Builder.CreateRet(RC);
  return Error::success();
}

///

struct SBTFinish
{
  ~SBTFinish()
  {
    sbt::SBT::finish();
  }
};

static void handleError(Error &&E)
{
  Error E2 = llvm::handleErrors(std::move(E),
    [](const SBTError &SE) {
      SE.log(errs());
      errs() << "\n";
      errs().flush();
      std::exit(EXIT_FAILURE);
    });

  if (E2) {
    logAllUnhandledErrors(std::move(E2), errs(), *BIN_NAME + ": error: ");
    std::exit(EXIT_FAILURE);
  }
}

} // sbt

static void test()
{
#if TEST
  // ExitOnError ExitOnErr;
  sbt::SBT::init();
  sbt::SBTFinish Finish;
  auto ExpObj = sbt::Object::create("sbt/test/hello.o");
  if (ExpObj)
    sbt::handleError(ExpObj.get().dump());
  else
    sbt::handleError(ExpObj.takeError());
  std::exit(EXIT_SUCCESS);
#endif
}

int main(int argc, char *argv[])
{
  test();

  // options
  cl::list<std::string> InputFiles(
      cl::Positional,
      cl::desc("<input object files>"),
      cl::OneOrMore);

  cl::opt<std::string> OutputFileOpt(
      "o",
      cl::desc("output filename"));

  // parse args
  cl::ParseCommandLineOptions(argc, argv);

  std::string OutputFile;
  if (OutputFileOpt.empty()) {
    OutputFile = "x86-" + InputFiles.front();
    // remove file extension
    OutputFile = OutputFile.substr(0, OutputFile.find_last_of('.')) + ".bc";
  } else
    OutputFile = OutputFileOpt;

  // start SBT
  sbt::SBT::init();
  sbt::SBTFinish Finish;

  // create SBT
  auto Exp = sbt::SBT::create(InputFiles, OutputFile);
  if (!Exp)
    sbt::handleError(Exp.takeError());
  sbt::SBT &SBT = Exp.get();

  // translate files
  sbt::handleError(SBT.run());

  // dump resulting IR
  SBT.dump();

  // abort while SBT translation is not finished
  errs() << "Error: Incomplete translation\n";
  std::exit(EXIT_FAILURE);

  // write IR to output file
  SBT.write();

  return EXIT_SUCCESS;
}
