#ifndef SBT_SBT_H
#define SBT_SBT_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>

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

  // ctor
  SBT(
    const llvm::cl::list<std::string>& inputFiles,
    const std::string& outputFile,
    llvm::Error& err);

  // allow move only
  SBT(SBT&&) = default;
  SBT(const SBT&) = delete;

  // dtor
  ~SBT() = default;

  // translate binaries
  llvm::Error run();

  // dump generated IR
  void dump() const;

  // write generated IR to output file
  void write();

  // generate Syscall handler
  llvm::Error genSCHandler();

private:
  std::string _outputFile;
  std::unique_ptr<llvm::LLVMContext> _context;
  std::unique_ptr<llvm::IRBuilder<>> _builder;
  std::unique_ptr<llvm::Module> _module;
  std::unique_ptr<Translator> _translator;

  // Target info
  const llvm::Target* _target;
  std::unique_ptr<const llvm::MCRegisterInfo> _mri;
  std::unique_ptr<const llvm::MCAsmInfo> _asmInfo;
  std::unique_ptr<const llvm::MCSubtargetInfo> _sti;
  std::unique_ptr<const llvm::MCObjectFileInfo> _mofi;
  std::unique_ptr<llvm::MCContext> _mc;
  std::unique_ptr<const llvm::MCDisassembler> _disAsm;
  std::unique_ptr<const llvm::MCInstrInfo> _mii;
  std::unique_ptr<llvm::MCInstPrinter> _instPrinter;

  /// private member functions

  // generate hello world IR (for test only)
  llvm::Error genHello();
};

} // sbt

#endif
