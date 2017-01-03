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
  static const bool ADD_NULL = true;
  static const bool CONSTANT = true;
  static const bool SIGNED = true;
  static const bool VAR_ARG = true;

public:
  static const std::string *BIN_NAME;

  // init/finalize class data
  static void init();
  static void finish();

  static llvm::Expected<SBT> create(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile);

  SBT(SBT &&) = default;
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

  /// private member functions

  // ctor
  SBT(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile,
    llvm::Error &Err);

  // translate one file
  llvm::Error translate(const std::string &File);

  // for test only - generate hello world IR
  llvm::Error genHello();
};

class SBTError : public llvm::ErrorInfo<SBTError>
{
public:
  SBTError(const std::string &FileName) :
    SS(new llvm::raw_string_ostream(S))
  {
    *SS << *SBT::BIN_NAME << ": '" << FileName << "': ";
  }

  SBTError(const SBTError &) = delete;

  SBTError(SBTError &&X) :
    S(std::move(X.SS->str())),
    SS(new llvm::raw_string_ostream(S))
  {
  }

  void log(llvm::raw_ostream &OS) const override
  {
    OS << SS->str();
  }

  std::error_code convertToErrorCode() const override
  {
    return llvm::inconvertibleErrorCode();
  }

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
  std::string S;
  std::unique_ptr<llvm::raw_string_ostream> SS;
};

}
