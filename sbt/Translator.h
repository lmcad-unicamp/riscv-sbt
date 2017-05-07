#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Constants.h"
#include "Map.h"
#include "Object.h"
#include "Register.h"
#include "Syscall.h"
#include "SBTError.h"

#include <llvm/MC/MCInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ELF.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FormatVariadic.h>

namespace llvm {
class Function;
class FunctionType;
class GlobalVariable;
class IntegerType;
class LLVMContext;
class MCDisassembler;
class MCInst;
class MCInstPrinter;
class MCSubtargetInfo;
class Module;
class Value;
}

namespace sbt {

class Translator
{
public:

  // ctor
  Translator(
    llvm::LLVMContext *ctx,
    llvm::IRBuilder<> *bldr,
    llvm::Module *mod);

  // move only
  Translator(Translator&&) = default;
  Translator(const Translator&) = delete;

  void addInputFile(const std::string& file)
  {
    _inputFiles.push_back(file);
  }

  // gen syscall handler
  llvm::Error genSCHandler();

  // translate input files
  llvm::Error translate();

  // setters

  void setOutputFile(const std::string& file)
  {
    _outputFile = file;
  }

  void setDisassembler(const llvm::MCDisassembler* d)
  {
    _disAsm = d;
  }

  void setInstPrinter(llvm::MCInstPrinter* p)
  {
    _instPrinter = p;
  }

  void setSTI(const llvm::MCSubtargetInfo* s)
  {
    _sti = s;
  }

private:
  // data

  // set by ctor
  llvm::LLVMContext* _ctx;
  llvm::IRBuilder<>* _builder;
  llvm::Module* _module;

  // set by sbt
  std::vector<std::string> _inputFiles;
  std::string _outputFile;
  const llvm::MCDisassembler* _disAsm = nullptr;
  llvm::MCInstPrinter* _instPrinter = nullptr;
  const llvm::MCSubtargetInfo* _sti = nullptr;

  // internal

  Syscall _sc;

  // stack
  llvm::GlobalVariable* _stack = nullptr;
  size_t _stackSize = 4096;

  // methods

  llvm::Error start();
  llvm::Error finish();

  // register file

  llvm::Error declOrBuildRegisterFile(bool decl);

  llvm::Error declRegisterFile()
  {
    return declOrBuildRegisterFile(true);
  }

  llvm::Error buildRegisterFile()
  {
    return declOrBuildRegisterFile(false);
  }

  // image
  llvm::Error buildStack();
};

} // sbt

#endif
