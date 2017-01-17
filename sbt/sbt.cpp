#include "sbt.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ELF.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

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

const std::string *SBT::BIN_NAME = nullptr;

void SBT::init()
{
  BIN_NAME = new std::string("riscv-sbt");

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
  delete BIN_NAME;
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
  Module(new llvm::Module("main", *Context))
{
  for (auto File : InputFilesList) {
    if (!sys::fs::exists(File)) {
      SBTError SE(File);
      SE << "No such file.\n";
      E = make_error<SBTError>(std::move(SE));
      return;
    }
    InputFiles.push_back(File);
  }
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

void SBT::write()
{
  std::error_code EC;
  raw_fd_ostream OS(OutputFile, EC, sys::fs::F_None);
  WriteBitcodeToFile(&*Module, OS);
  OS.flush();
}

// private helper functions

typedef std::vector<std::pair<uint64_t, StringRef>> SymbolVec;

// get all symbols present in Obj that belong to this Section
static Expected<SymbolVec>
getSymbolsList(const object::ObjectFile *Obj, const object::SectionRef &Section)
{
  uint64_t SectionAddr = Section.getAddress();

  std::error_code EC;
  // Make a list of all the symbols in this section.
  SymbolVec Symbols;
  for (auto Symbol : Obj->symbols()) {
    // skip symbols not present in this Section
    if (!Section.containsSymbol(Symbol))
      continue;

    // address
    auto Exp = Symbol.getAddress();
    if (!Exp)
      return Exp.takeError();
    uint64_t &Address = Exp.get();
    Address -= SectionAddr;

    // name
    auto Exp2 = Symbol.getName();
    if (!Exp2)
      return Exp2.takeError();
    StringRef &Name = Exp2.get();
    Symbols.push_back(std::make_pair(Address, Name));
  }

  // Sort the symbols by address, just in case they didn't come in that way.
  array_pod_sort(Symbols.begin(), Symbols.end());
  return Symbols;
}

static uint64_t getELFOffset(const object::SectionRef &Section)
{
  object::DataRefImpl Sec = Section.getRawDataRefImpl();
  auto sec = reinterpret_cast<
    const object::Elf_Shdr_Impl<
      object::ELFType<support::little, false>> *>(Sec.p);
  return sec->sh_offset;
}

Error SBT::translate(const std::string &File)
{
  // prepare error handling
  SBTError SE(File);
  auto E = [&SE]() {
    return make_error<SBTError>(std::move(SE));
  };

  // attempt to open the binary.
  auto Exp = object::createBinary(File);
  if (!Exp)
    return Exp.takeError();
  object::OwningBinary<object::Binary> &OB = Exp.get();
  object::Binary *Bin = OB.getBinary();

  // for now, only object files are supported
  if (!object::ObjectFile::classof(Bin)) {
    SE << "Unrecognized file type.\n";
    return E();
  }

  auto Obj = dyn_cast<object::ObjectFile>(Bin);
  log() << Obj->getFileName() << ": file format: " << Obj->getFileFormatName()
         << "\n";

  // get target
  Triple Triple("riscv32-unknown-elf");
  Triple.setArch(Triple::ArchType(Obj->getArch()));
  std::string TripleName = Triple.getTriple();
  log() << Obj->getFileName() << ": Triple: " << TripleName << "\n";

  std::string StrError;
  const Target *Target = &getTheRISCVMaster32Target();
  // const Target *Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!Target) {
    SE << "Target not found: " << TripleName << ": " << StrError << "\n";
    return E();
  }

  std::unique_ptr<const MCRegisterInfo> MRI(
      Target->createMCRegInfo(TripleName));
  if (!MRI) {
    SE << "No register info for target " << TripleName << "\n";
    return E();
  }

  // Set up disassembler.
  std::unique_ptr<const MCAsmInfo> AsmInfo(
      Target->createMCAsmInfo(*MRI, TripleName));
  if (!AsmInfo) {
    SE << "No assembly info for target " << TripleName << "\n";
    return E();
  }

  SubtargetFeatures Features;
  std::unique_ptr<const MCSubtargetInfo> STI(
      Target->createMCSubtargetInfo(TripleName, "", Features.getString()));
  if (!STI) {
    SE << "No subtarget info for target " << TripleName << "\n";
    return E();
  }

  std::unique_ptr<const MCObjectFileInfo> MOFI(new MCObjectFileInfo);
  MCContext Ctx(AsmInfo.get(), MRI.get(), MOFI.get());

  std::unique_ptr<const MCDisassembler> DisAsm(
      Target->createMCDisassembler(*STI, Ctx));
  if (!DisAsm) {
    SE << "No disassembler for target " << TripleName << "\n";
    return E();
  }

  std::unique_ptr<const MCInstrInfo> MII(Target->createMCInstrInfo());
  if (!MII) {
    SE << "No instruction info for target " << TripleName << "\n";
    return E();
  }

  std::unique_ptr<MCInstPrinter> InstPrinter(
    Target->createMCInstPrinter(Triple, 0, *AsmInfo, *MII, *MRI));
  if (!InstPrinter) {
    SE << "No instruction printer for target " << TripleName << "\n";
    return E();
  }

  // for each section
  for (const object::SectionRef &Section : Obj->sections()) {
    // skip non code sections
    if (!Section.isText())
      continue;

    // get section name
    StringRef SectionName;
    std::error_code EC = Section.getName(SectionName);
    if (EC) {
      SE << "Failed to get Section Name";
      return E();
    }
#ifndef NDEBUG
    log() << "Disassembly of section " << SectionName << ":\n";
#endif

    // get section bytes
    StringRef BytesStr;
    EC = Section.getContents(BytesStr);
    if (EC) {
      SE << "Failed to get Section Contents";
      return E();
    }
    ArrayRef<uint8_t> Bytes(reinterpret_cast<const uint8_t *>(BytesStr.data()),
                            BytesStr.size());

    // Make a list of all the symbols in this section.
    auto Exp = getSymbolsList(Obj, Section);
    if (!Exp)
      return Exp.takeError();
    SymbolVec &Symbols = Exp.get();
    // If the section has no symbols just insert a dummy one and disassemble
    // the whole section.
    if (Symbols.empty())
      Symbols.push_back(std::make_pair(0, SectionName));

    uint64_t SectionAddr = Section.getAddress();
    uint64_t SectSize = Section.getSize();
    // Disassemble symbol by symbol.
    for (unsigned si = 0, se = Symbols.size(); si != se; ++si) {
      const SymbolVec::value_type &Symbol = Symbols[si];
      uint64_t Start = Symbol.first;
      uint64_t End;
      const StringRef &SymbolName = Symbol.second;

      // The end is either the size of the section or the beginning of the next
      // symbol.
      if (si == se - 1)
        End = SectSize;
      // Make sure this symbol takes up space.
      else if (Symbols[si + 1].first != Start)
        End = Symbols[si + 1].first - 1;
      else
        // This symbol has the same address as the next symbol. Skip it.
        continue;

#ifndef NDEBUG
      outs() << SymbolName << ":\n";
      raw_ostream &DebugOut = dbgs();
#else
      raw_ostream &DebugOut = nulls();
#endif

      // Relocatable object
      uint64_t ELFOffset = SectionAddr;
      if (SectionAddr == 0)
        ELFOffset = getELFOffset(Section);

      if (SymbolName == "main")
        ; // TODO start main
      else {
        Error E = startFunction(SymbolName, Start + ELFOffset);
        if (E)
          return E;
      }

      // for each instruction
      uint64_t Size;
      for (uint64_t Index = Start; Index < End; Index += Size) {
        uint64_t Addr = Index + ELFOffset;

        MCInst Inst;
        MCDisassembler::DecodeStatus st =
          DisAsm->getInstruction(Inst, Size, Bytes.slice(Index),
            SectionAddr + Index, DebugOut, nulls());
        if (st == MCDisassembler::DecodeStatus::Success) {
#ifndef NDEBUG
          InstPrinter->printInst(&Inst, outs(), "", *STI);
          outs() << "\n";
#endif
          Error E = translate(Inst, Addr);
          if (E)
            return E;
        } else {
          SE << "Invalid instruction encoding\n";
          return E();
        }
      } // for each instruction
      // TODO finish function
    } // for each symbol
  } // for each section
  // TODO finish module

  return Error::success();
}

Error SBT::translate(const MCInst &Instr, uint64_t Addr)
{
  return Error::success();
}

Error SBT::startFunction(StringRef Name, uint64_t Addr)
{
  // FunctionAddrs.push_back(Addr);

  // Create a function with no parameters
  FunctionType *FT =
    FunctionType::get(Type::getVoidTy(*Context), !VAR_ARG);
  Function *F =
    Function::Create(FT,
      Function::ExternalLinkage, Name, &*Module);

  // BB
  Twine BBName = Twine("bb").concat(Twine::utohexstr(Addr));
  BasicBlock *BB =
    BasicBlock::Create(*Context, BBName, F);
  Builder.SetInsertPoint(BB);

  if (FirstFunction) {
    FirstFunction = false;
    buildRegisterFile();
    // CurFunAddr = Addr;
  }

  return Error::success();
}

void SBT::buildRegisterFile()
{
  Type *Ty = Type::getInt32Ty(*Context);
  Constant *X0 = ConstantInt::get(Ty, 0u);
  X[0] = new GlobalVariable(*Module, Ty, CONSTANT,
    GlobalValue::ExternalLinkage, X0, "X0");

  for (int I = 1; I < 32; ++I) {
    std::string RegName = Twine("X").concat(Twine(I)).str();
    X[I] = new GlobalVariable(*Module, Ty, !CONSTANT,
        GlobalValue::ExternalLinkage, X0, RegName);
  }
}

/// SBTError ///

char SBTError::ID = 'S';

///

void handleError(Error &&E)
{
  Error E2 = llvm::handleErrors(std::move(E),
    [](const SBTError &SE) {
      // errs() << "SBTError:\n";
      SE.log(errs());
      errs() << "\n";
      errs().flush();
      std::exit(EXIT_FAILURE);
    });

  if (E2) {
    logAllUnhandledErrors(std::move(E2), errs(), *SBT::BIN_NAME + ": error: ");
    std::exit(EXIT_FAILURE);
  }
}

} // sbt


int main(int argc, char *argv[])
{
  // test();
  // ExitOnError ExitOnErr;

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

  struct SBTFinish
  {
    ~SBTFinish()
    {
      sbt::SBT::finish();
    }
  } Finish;

  Expected<sbt::SBT> Exp = sbt::SBT::create(InputFiles, OutputFile);
  if (!Exp)
    sbt::handleError(Exp.takeError());
  sbt::SBT &SBT = Exp.get();

  sbt::handleError(SBT.run());
  SBT.dump();

  errs() << "Error: Incomplete translation\n";
  std::exit(EXIT_FAILURE);
  SBT.write();

  return EXIT_SUCCESS;
}
