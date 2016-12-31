#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
}

namespace sbt {

class SBT
{
  static const bool ADD_NULL = true;
  static const bool CONSTANT = true;
  static const bool SIGNED = true;
  static const bool VAR_ARG = true;

  static const std::string BIN_NAME;

public:
  SBT(
    const llvm::cl::list<std::string> &inputFiles,
    const std::string &outputFile);

  ~SBT() = default;

  int run();

  void dump() const
  {
    _module.dump();
  }

  void write();

  operator bool() const
  {
    return _valid;
  }

private:
  std::vector<std::string> _inputFiles;
  std::string _outputFile;

  llvm::LLVMContext _context;
  llvm::IRBuilder<> _builder;
  llvm::Module _module;

  bool _valid = false;

  ///

  int translate(const std::string &file);
  int genHello();   // for test only
};

using namespace std;
using namespace llvm;

const string SBT::BIN_NAME = "riscv-sbt";

SBT::SBT(
  const cl::list<string> &inputFiles,
  const string &outputFile)
  :
  _outputFile(outputFile),
  _builder(_context),
  _module("main", _context)
{
  // print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal("riscv-sbt");

  for (auto file : inputFiles) {
    if (!sys::fs::exists(file)) {
      errs() << BIN_NAME << ": '" << file << "': No such file\n";
      return;
    }
    _inputFiles.push_back(file);
  }

  _valid = true;
}

int SBT::translate(const string &file)
{
  auto err = [&file]() -> raw_ostream & {
    errs() << BIN_NAME << ": '" << file << "': ";
    return errs();
  };

  // attempt to open the binary.
  Expected<object::OwningBinary<object::Binary>> exp = object::createBinary(file);
  if (!exp) {
    logAllUnhandledErrors(exp.takeError(), errs(), BIN_NAME + ": '" + file + "': ");
    return 1;
  }
  object::OwningBinary<object::Binary> &ob = exp.get();
  object::Binary *bin = ob.getBinary();

  if (!object::ObjectFile::classof(bin)) {
    err() << "Unrecognized file type.\n";
    return 1;
  }

  object::ObjectFile *obj = dyn_cast<object::ObjectFile>(bin);

  outs() << obj->getFileName() << ": file format: " << obj->getFileFormatName()
         << "\n";

  // get target
  Triple triple("riscv32-unknown-elf");
  triple.setArch(Triple::ArchType(obj->getArch()));
  string tripleName = triple.getTriple();
  outs() << "triple: " << tripleName << "\n";

  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();

  string error;
  const Target *target = TargetRegistry::lookupTarget(tripleName, error);
  if (!target) {
    err() << "target not found: " << tripleName
          << ": " << error << "\n";
    return 1;
  }

  return 0;
}

int SBT::genHello()
{
  // constants
  static const int SYS_EXIT = 1;
  static const int SYS_WRITE = 4;

  // types
  Type *Int8 = Type::getInt8Ty(_context);
  Type *Int32 = Type::getInt32Ty(_context);

  // syscall
  FunctionType *ft_syscall =
    FunctionType::get(Int32, { Int32, Int32 }, VAR_ARG);
  Function *f_syscall = Function::Create(ft_syscall,
      Function::ExternalLinkage, "syscall", &_module);

  // set data
  string hello("Hello, World!\n");
  GlobalVariable *msg =
    new GlobalVariable(
      _module,
      ArrayType::get(Int8, hello.size()),
      CONSTANT,
      GlobalVariable::PrivateLinkage,
      ConstantDataArray::getString(_context, hello.c_str(), !ADD_NULL),
      "msg");

  // _start
  FunctionType *ft_start = FunctionType::get(Int32, {}, !VAR_ARG);
  Function *f_start =
    Function::Create(ft_start, Function::ExternalLinkage, "_start", &_module);

  // entry basic block
  BasicBlock *bb = BasicBlock::Create(_context, "entry", f_start);
  _builder.SetInsertPoint(bb);

  // call write
  Value *sc = ConstantInt::get(Int32, APInt(32, SYS_WRITE, SIGNED));
  Value *fd = ConstantInt::get(Int32, APInt(32, 1, SIGNED));
  Value *ptr = msg;
  Value *len = ConstantInt::get(Int32, APInt(32, hello.size(), SIGNED));
  vector<Value *> args = { sc, fd, ptr, len };
  _builder.CreateCall(f_syscall, args);

  // call exit
  sc = ConstantInt::get(Int32, APInt(32, SYS_EXIT, SIGNED));
  Value *rc = ConstantInt::get(Int32, APInt(32, 0, SIGNED));
  args = { sc, rc };
  _builder.CreateCall(f_syscall, args);
  _builder.CreateRet(rc);
  return 0;
}

int SBT::run()
{
  // genHello();

  int rc;
  for (auto &f : _inputFiles) {
    rc = translate(f);
    if (rc)
      return rc;
  }
  return 0;
}

void SBT::write()
{
  std::error_code ec;
  llvm::raw_fd_ostream os(_outputFile, ec, sys::fs::F_None);
  llvm::WriteBitcodeToFile(&_module, os);
  os.flush();
}

} // sbt

int main(int argc, char *argv[])
{
  using namespace llvm;
  using namespace std;

  // options
  cl::list<string> inputFiles(
      cl::Positional,
      cl::desc("<input object files>"),
      cl::OneOrMore);

  cl::opt<string> outputFileOpt(
      "o",
      cl::desc("output filename"));

  // parse args
  cl::ParseCommandLineOptions(argc, argv);

  string outputFile;
  if (outputFileOpt.empty()) {
    outputFile = "x86-" + inputFiles.front();
    // remove file extension
    outputFile = outputFile.substr(0, outputFile.find_last_of('.')) + ".bc";
  } else
    outputFile = outputFileOpt;

  // start SBT
  sbt::SBT sbt(inputFiles, outputFile);
  if (!sbt)
    return 1;

  if (sbt.run())
    return 1;

  sbt.dump();
  sbt.write();
  return 0;
}
