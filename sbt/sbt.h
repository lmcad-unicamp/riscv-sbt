#ifndef SBT_SBT_H
#define SBT_SBT_H

#include "Context.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>

#include <memory>
#include <string>

namespace sbt {

class Translator;
class Options;

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
    const Options& opts,
    llvm::Error& err);

  // allow move only
  SBT(SBT&&) = default;
  SBT(const SBT&) = delete;
  SBT& operator=(const SBT&) = delete;
  SBT& operator=(SBT&&) = delete;

  // translate binaries
  llvm::Error run();

  // dump generated IR
  void dump() const;

  // write generated IR to output file
  void write();

  // generate syscall handler
  llvm::Error genSCHandler();

private:
  std::string _outputFile;
  std::unique_ptr<llvm::LLVMContext> _context;
  std::unique_ptr<llvm::Module> _module;
  std::unique_ptr<llvm::IRBuilder<>> _builder;
  std::unique_ptr<Context> _ctx;
  std::unique_ptr<Translator> _translator;
};

} // sbt

#endif
