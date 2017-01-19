#ifndef SBT_SBT_H
#define SBT_SBT_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/CommandLine.h>

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class LLVMContext;
class MCAsmInfo;
class MCContext;
class MCDisassembler;
class MCInstPrinter;
class MCInstrInfo;
class MCObjectFileInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class Module;
class Target;
}

namespace sbt {

class Translator;

class SBT
{
public:
  // initialize/finalize class data
  static void init();
  static void finish();

  // SBT factory
  static llvm::Expected<SBT>
  create(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile);

  // allow move
  SBT(SBT &&) = default;
  // disallow copy
  SBT(const SBT &) = delete;

  // dtor
  ~SBT() = default;

  // translate binaries
  llvm::Error run();

  // dump generated IR
  void dump() const;

  // write generated IR to output file
  void write();

private:
  std::vector<std::string> InputFiles;
  std::string OutputFile;

  std::unique_ptr<llvm::LLVMContext> Context;
  llvm::IRBuilder<> Builder;
  std::unique_ptr<llvm::Module> Module;
  std::unique_ptr<Translator> SBTTranslator;

  // Target info
  const llvm::Target *Target;
  std::unique_ptr<const llvm::MCRegisterInfo> MRI;
  std::unique_ptr<const llvm::MCAsmInfo> AsmInfo;
  std::unique_ptr<const llvm::MCSubtargetInfo> STI;
  std::unique_ptr<const llvm::MCObjectFileInfo> MOFI;
  std::unique_ptr<llvm::MCContext> MC;
  std::unique_ptr<const llvm::MCDisassembler> DisAsm;
  std::unique_ptr<const llvm::MCInstrInfo> MII;
  std::unique_ptr<llvm::MCInstPrinter> InstPrinter;

  /// private member functions

  // ctor
  SBT(
    const llvm::cl::list<std::string> &InputFiles,
    const std::string &OutputFile,
    llvm::Error &Err);

  // translate one file
  llvm::Error translate(const std::string &File);

  // generate hello world IR (for test only)
  llvm::Error genHello();
};

} // sbt

#endif
