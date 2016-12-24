#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <string>
#include <vector>

namespace sbt {

class SBT
{
public:
  static const bool ADD_NULL = true;
  static const bool CONSTANT = true;
  static const bool SIGNED = true;
  static const bool VAR_ARG = true;

  SBT(
    const llvm::cl::list<std::string> &inputFiles,
    const std::string &outputFile);

  ~SBT() = default;

  void run();

  void dump() const
  {
    _module.dump();
  }

  void write();

private:
  std::vector<std::string> _inputFiles;
  std::string _outputFile;

  llvm::LLVMContext _context;
  llvm::IRBuilder<> _builder;
  llvm::Module _module;
};

using namespace std;
using namespace llvm;

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

  for (auto e : inputFiles)
    _inputFiles.push_back(e);
}

void SBT::run()
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
  sbt.run();
  sbt.dump();
  sbt.write();
  return 0;
}
