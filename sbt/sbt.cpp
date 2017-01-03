#include "sbt.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Object/Binary.h>
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


Error SBT::translate(const std::string &File)
{
  Error E = make_error<SBTError>(File);
  SBTError &SE = cast<SBTError>(E);

  // attempt to open the binary.
  Expected<object::OwningBinary<object::Binary>> Exp =
    object::createBinary(File);
  if (!Exp)
    return Exp.takeError();
  object::OwningBinary<object::Binary> &OB = Exp.get();
  object::Binary *Bin = OB.getBinary();

  if (!object::ObjectFile::classof(Bin)) {
    SE << "Unrecognized file type.\n";
    return E;
  }

  object::ObjectFile *obj = dyn_cast<object::ObjectFile>(Bin);

  outs() << obj->getFileName() << ": file format: " << obj->getFileFormatName()
         << "\n";

  // get target
  Triple triple("riscv32-unknown-elf");
  triple.setArch(Triple::ArchType(obj->getArch()));
  std::string tripleName = triple.getTriple();
  outs() << "triple: " << tripleName << "\n";

  std::string StrError;
  const Target *Target = TargetRegistry::lookupTarget(tripleName, StrError);
  if (!Target) {
    SE << "target not found: " << tripleName << ": " << StrError << "\n";
    return E;
  }

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


/// SBTError ///

char SBTError::ID = 'S';

///

void handleError(Error &&E)
{
  Error E2 = llvm::handleErrors(std::move(E),
    [](const SBTError &SE) {
      SE.log(errs());
    });

  if (E2)
    logAllUnhandledErrors(std::move(E2), errs(), *SBT::BIN_NAME + ": ");

  std::exit(EXIT_FAILURE);
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
  sbt::SBT &SBT = Exp.get();
  if (!Exp)
    sbt::handleError(Exp.takeError());

  sbt::handleError(SBT.run());
  SBT.dump();
  SBT.write();

  return EXIT_SUCCESS;
}
