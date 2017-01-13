#include "sbt.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ELF.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

using namespace llvm;

namespace sbt {

void SBT::init()
{
  BIN_NAME = new std::string("riscv-sbt");

  // print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(*BIN_NAME);

  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();
}

void SBT::finish()
{
  delete BIN_NAME;
  llvm_shutdown();
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

const std::string *SBT::BIN_NAME = nullptr;

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

static Expected<std::vector<std::pair<uint64_t, StringRef>>>
getSymbolsList(const object::ObjectFile *Obj, const object::SectionRef &Section)
{
  uint64_t SectionAddr = Section.getAddress();

  std::error_code EC;
  // Make a list of all the symbols in this section.
  std::vector<std::pair<uint64_t, StringRef>> Symbols;
  for (auto Symbol : Obj->symbols()) {
    if (!Section.containsSymbol(Symbol))
      continue;

    auto Exp = Symbol.getAddress();
    if (!Exp)
      return Exp.takeError();
    auto &Address = Exp.get();
    Address -= SectionAddr;

    auto Exp2 = Symbol.getName();
    if (!Exp2)
      return Exp2.takeError();
    auto &Name = Exp2.get();
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
  SBTError SE(File);
  auto E = [&SE](){
    return make_error<SBTError>(std::move(SE));
  };

  // attempt to open the binary.
  Expected<object::OwningBinary<object::Binary>> Exp =
    object::createBinary(File);
  if (!Exp)
    return Exp.takeError();
  object::OwningBinary<object::Binary> &OB = Exp.get();
  object::Binary *Bin = OB.getBinary();

  if (!object::ObjectFile::classof(Bin)) {
    SE << "Unrecognized file type.\n";
    return E();
  }

  object::ObjectFile *Obj = dyn_cast<object::ObjectFile>(Bin);

  outs() << Obj->getFileName() << ": file format: " << Obj->getFileFormatName()
         << "\n";

  // get target
  Triple Triple("riscv32-unknown-elf");
  Triple.setArch(Triple::ArchType(Obj->getArch()));
  std::string TripleName = Triple.getTriple();
  outs() << "Triple: " << TripleName << "\n";

  std::string StrError;
  const Target *Target = TargetRegistry::lookupTarget(TripleName, StrError);
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

  // sections
  for (const object::SectionRef &Section : Obj->sections()) {
    if (!Section.isText())
      continue;

    uint64_t SectionAddr = Section.getAddress();

    StringRef SegmentName = "";
    if (const object::MachOObjectFile *MachO =
        dyn_cast<const object::MachOObjectFile>(Obj)) {
      object::DataRefImpl DR = Section.getRawDataRefImpl();
      SegmentName = MachO->getSectionFinalSegmentName(DR);
    }
    StringRef Name;
    std::error_code EC = Section.getName(Name);
    if (EC) {
      SE << "Failed to get Section Name";
      return E();
    }
#ifndef NDEBUG
    outs() << "Disassembly of section ";
    if (!SegmentName.empty())
      outs() << SegmentName << ",";
    outs() << Name << ':';
#endif

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
    auto &Symbols = Exp.get();
    // If the section has no symbols just insert a dummy one and disassemble
    // the whole section.
    if (Symbols.empty())
      Symbols.push_back(std::make_pair(0, Name));

    uint64_t SectSize = Section.getSize();
    // Disassemble symbol by symbol.
    for (unsigned si = 0, se = Symbols.size(); si != se; ++si) {
      uint64_t Start = Symbols[si].first;
      uint64_t End;

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
      outs() << '\n' << Symbols[si].second << ":\n";
#endif

#ifndef NDEBUG
      raw_ostream &DebugOut = DebugFlag ? dbgs() : nulls();
#else
      raw_ostream &DebugOut = nulls();
#endif

      /*
      uint64_t eoffset = SectionAddr;
      // Relocatable object
      if (SectionAddr == 0)
        // TODO handle errors
        eoffset = getELFOffset(Section);
      */

      /*
      if (Symbols[si].second == "main")
        IP->StartMainFunction(Start + eoffset);
      else
        IP->StartFunction(
            Twine("a").concat(Twine::utohexstr(Start + eoffset)).str(),
            Start + eoffset);
       */

      // instructions
      uint64_t Size;
      outs() << "SectionAddr = " << SectionAddr << "\n";
      outs() << "Bytes = " << Bytes.size() << "\n";
      for (uint64_t Index = Start; Index < End; Index += Size) {
        outs() << "Index = " << Index << "\n";

        MCInst Inst;

        // IP->UpdateCurAddr(Index + eoffset);
        MCDisassembler::DecodeStatus st =
          DisAsm->getInstruction(Inst, Size, Bytes.slice(Index),
            SectionAddr + Index, DebugOut, nulls());
        if (st == MCDisassembler::DecodeStatus::Success) {
          outs() << "*\n";
          Inst.print(outs());
          /*
#ifndef NDEBUG
          outs() << format("%8" PRIx64 ":", eoffset + Index);
          outs() << "\t";
          DumpBytes(StringRef(BytesStr.data() + Index, Size));
#endif
          IP->printInst(&Inst, outs(), "");
#ifdef NDEBUG
          if (++NumProcessed % 10000 == 0) {
            outs() << ".";
          }
#endif
#ifndef NDEBUG
          outs() << "\n";
#endif
        */
        } else {
          SE << "Invalid instruction encoding\n";
          return E();
        }
      } // instructions
      // IP->FinishFunction();
    } // symbols
  } // sections
  // IP->FinishModule();
  // OptimizeAndWriteBitcode(&*IP);

  return Error::success();
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
