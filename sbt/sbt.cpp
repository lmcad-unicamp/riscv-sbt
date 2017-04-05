#include "sbt.h"

#include "Constants.h"
#include "Object.h"
#include "SBTError.h"
#include "Translator.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
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
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
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

void SBT::init()
{
  Object::init();

  // Print stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal("");

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
  Object::finish();
}

Expected<SBT> SBT::create(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile)
{
  Error Err = Error::success();
  llvm::consumeError(std::move(Err));
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
  Builder(new IRBuilder<>(*Context)),
  Module(new llvm::Module("main", *Context)),
  SBTTranslator(new Translator(&*Context, &*Builder, &*Module))
{
  for (auto File : InputFilesList) {
    if (!sys::fs::exists(File)) {
      SBTError SE(File);
      SE << "No such file.";
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
  // logs() << "Triple: " << TripleName << "\n";

  std::string StrError;
  Target = &getTheRISCVMaster32Target();
  // Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!Target) {
    SE << "Target not found: " << TripleName << ": " << StrError;
    error(SE);
    return;
  }

  MRI.reset(Target->createMCRegInfo(TripleName));
  if (!MRI) {
    SE << "No register info for target " << TripleName;
    error(SE);
    return;
  }

  // Set up disassembler.
  AsmInfo.reset(Target->createMCAsmInfo(*MRI, TripleName));
  if (!AsmInfo) {
    SE << "No assembly info for target " << TripleName;
    error(SE);
    return;
  }

  SubtargetFeatures Features;
  STI.reset(
      Target->createMCSubtargetInfo(TripleName, "", Features.getString()));
  if (!STI) {
    SE << "No subtarget info for target " << TripleName;
    error(SE);
    return;
  }

  MOFI.reset(new MCObjectFileInfo);
  MC.reset(new MCContext(AsmInfo.get(), MRI.get(), MOFI.get()));
  DisAsm.reset(Target->createMCDisassembler(*STI, *MC));
  if (!DisAsm) {
    SE << "No disassembler for target " << TripleName;
    error(SE);
  }

  MII.reset(Target->createMCInstrInfo());
  if (!MII) {
    SE << "No instruction info for target " << TripleName;
    error(SE);
    return;
  }

  InstPrinter.reset(
    Target->createMCInstPrinter(Triple, 0, *AsmInfo, *MII, *MRI));
  if (!InstPrinter) {
    SE << "No instruction printer for target " << TripleName;
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

  if (llvm::verifyModule(*Module, &DBGS)) {
    SBTError SE;
    SE << "Translation produced invalid bitcode!";
    dump();
    return error(SE);
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


llvm::Error SBT::translate(const std::string &File)
{
  // parse object file
  auto ExpObj = sbt::create<Object>(File);
  if (!ExpObj)
    return ExpObj.takeError();
  Object *Obj = &ExpObj.get();
  SBTTranslator->setCurObj(Obj);

  SBTTranslator->setInstPrinter(&*InstPrinter);
  SBTTranslator->setDisassembler(&*DisAsm);
  SBTTranslator->setSTI(&*STI);

  // start module
  if (Error E = SBTTranslator->startModule())
    return E;

  // translate each section
  for (ConstSectionPtr Sec : Obj->sections()) {
    if (auto E = SBTTranslator->translateSection(Sec))
      return E;

    // finish any pending function
    if (Error E = SBTTranslator->finishFunction())
      return E;
  }

  // finish module
  if (Error E = SBTTranslator->finishModule())
    return E;

  return Error::success();
}


llvm::Error SBT::genHello()
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
  Builder->SetInsertPoint(BB);

  // call write
  Value *SC = ConstantInt::get(Int32, APInt(32, SYS_WRITE, SIGNED));
  Value *FD = ConstantInt::get(Int32, APInt(32, 1, SIGNED));
  Value *Ptr = Msg;
  Value *Len = ConstantInt::get(Int32, APInt(32, Hello.size(), SIGNED));
  std::vector<Value *> Args = { SC, FD, Ptr, Len };
  Builder->CreateCall(FSyscall, Args);

  // call exit
  SC = ConstantInt::get(Int32, APInt(32, SYS_EXIT, SIGNED));
  Value *RC = ConstantInt::get(Int32, APInt(32, 0, SIGNED));
  Args = { SC, RC };
  Builder->CreateCall(FSyscall, Args);
  Builder->CreateRet(RC);

  // XXX Used for testing only, remove
  Module->dump();
  std::exit(EXIT_FAILURE);

  return Error::success();
}

Error SBT::genSCHandler()
{
  if (auto E = SBTTranslator->genSCHandler())
    return E;
  write();
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
#if 1
  using namespace sbt;

  // ExitOnError ExitOnErr;
  SBT::init();
  SBTFinish Finish;
  StringRef FilePath = "sbt/test/rv32-main.o";
  auto ExpObj = create<Object>(FilePath);
  if (!ExpObj)
    handleError(ExpObj.takeError());
  else
    ExpObj.get().dump();
  std::exit(EXIT_FAILURE);
#endif
}

int main(int argc, char *argv[])
{
  // initialize constants
  sbt::initConstants();
  struct DestroyConstants {
    ~DestroyConstants() {
      sbt::destroyConstants();
    }
  } DestroyConstants;

  // options
  cl::list<std::string> InputFiles(
      cl::Positional,
      cl::desc("<input object files>"),
      cl::ZeroOrMore);

  cl::opt<std::string> OutputFileOpt(
      "o",
      cl::desc("output filename"));

  cl::opt<bool> GenSCHandlerOpt(
      "gen-sc-handler",
      cl::desc("generate syscall handler"));

  cl::opt<bool> TestOpt("test");

  // parse args
  cl::ParseCommandLineOptions(argc, argv);

  // gen syscall handlers
  if (GenSCHandlerOpt) {
    if (OutputFileOpt.empty()) {
      errs() << *sbt::BIN_NAME
        << ": Output file not specified.\n";
      return 1;
    }
  // debug test
  } else if (TestOpt) {
    test();
  // translate
  } else {
    if (InputFiles.empty()) {
      errs() << *sbt::BIN_NAME << ": No input files.\n";
      return 1;
    }
  }

  // set output file
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
  if (GenSCHandlerOpt) {
    sbt::handleError(SBT.genSCHandler());
    return EXIT_SUCCESS;
  } else
    sbt::handleError(SBT.run());

  // dump resulting IR
  SBT.dump();

  // write IR to output file
  SBT.write();

  return EXIT_SUCCESS;
}
