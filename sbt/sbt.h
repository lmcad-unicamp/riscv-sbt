#include <llvm/MC/MCInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <string>
#include <vector>

namespace sbt {

class SBT
{
  // private constants
  // (only the make the code easier to read)
  static const bool ADD_NULL = true;
  static const bool CONSTANT = true;
  static const bool ERROR = true;
  static const bool SIGNED = true;
  static const bool VAR_ARG = true;

public:
  // name of the SBT binary/executable
  static const std::string *BIN_NAME;

  // initialize/finalize class data
  static void init();
  static void finish();

  // SBT factory
  static llvm::Expected<SBT> create(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile);

  // allow move
  SBT(SBT &&) = default;
  // disallow copy
  SBT(const SBT &) = delete;

  // dtor
  ~SBT() = default;

  // run - translate binaries
  llvm::Error run();

  // dump generated IR
  void dump() const
  {
    Module->dump();
  }

  // write generated IR to output file
  void write();

private:
  std::vector<std::string> InputFiles;
  std::string OutputFile;

  std::unique_ptr<llvm::LLVMContext> Context;
  llvm::IRBuilder<> Builder;
  std::unique_ptr<llvm::Module> Module;

  bool FirstFunction = true;
  llvm::GlobalVariable *X[32];

  /// private member functions

  // ctor
  SBT(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile,
    llvm::Error &Err);

  // translate one file
  llvm::Error translate(const std::string &File);
  // translate one instruction
  llvm::Error translate(const llvm::MCInst &Instr, uint64_t Addr);

  llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);
  void buildRegisterFile();

  llvm::raw_ostream &log(bool error = false) const
  {
    return (error? llvm::errs() : llvm::outs()) << *BIN_NAME << ": ";
  }

  // for test only - generate hello world IR
  llvm::Error genHello();
};

// Our custom error class
class SBTError : public llvm::ErrorInfo<SBTError>
{
public:
  // ctor
  // FileName - name of the input file that was being processed
  //            when the error happened
  SBTError(const std::string &FileName) :
    SS(new llvm::raw_string_ostream(S))
  {
    // error format: <sbt>: error: '<file>': <msg>
    *SS << *SBT::BIN_NAME << ": error: '" << FileName << "': ";
  }

  // disallow copy
  SBTError(const SBTError &) = delete;

  // move
  SBTError(SBTError &&X) :
    S(std::move(X.SS->str())),
    SS(new llvm::raw_string_ostream(S))
  {
  }

  // log error
  void log(llvm::raw_ostream &OS) const override
  {
    OS << SS->str();
  }

  // unused error_code conversion compatibility method
  std::error_code convertToErrorCode() const override
  {
    return llvm::inconvertibleErrorCode();
  }

  // stream insertion overloads to make it easy
  // to build the error message
  template <typename T>
  llvm::raw_string_ostream &operator<<(const T &&Val)
  {
    *SS << Val;
    return *SS;
  }

  llvm::raw_string_ostream &operator<<(const char *Val)
  {
    *SS << Val;
    return *SS;
  }

  // Used by ErrorInfo::classID.
  static char ID;

private:
  std::string S;                                  // error message
  std::unique_ptr<llvm::raw_string_ostream> SS;   // string stream
};

} // sbt
