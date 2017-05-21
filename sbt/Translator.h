#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Context.h"
#include "Function.h"

#include <llvm/Support/Error.h>

namespace llvm {
class GlobalVariable;
class MCAsmInfo;
class MCContext;
class MCDisassembler;
class MCInst;
class MCInstPrinter;
class MCInstrInfo;
class MCObjectFileInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;
}

namespace sbt {

class Translator
{
public:

  // ctor
  Translator(Context* ctx);

  // move only
  Translator(Translator&&) = default;
  Translator(const Translator&) = delete;

  ~Translator();

  void addInputFile(const std::string& file)
  {
    _inputFiles.push_back(file);
  }

  // gen syscall handler
  llvm::Error genSCHandler();

  // translate input files
  llvm::Error translate();

  llvm::Expected<uint64_t> import(const std::string& func);

  // setters

  void setOutputFile(const std::string& file)
  {
    _outputFile = file;
  }

private:
  // data

  // set by ctor
  Context* _ctx;

  // set by sbt
  std::vector<std::string> _inputFiles;
  std::string _outputFile;


  // target info
  const llvm::Target* _target;
  std::unique_ptr<const llvm::MCRegisterInfo> _mri;
  std::unique_ptr<const llvm::MCAsmInfo> _asmInfo;
  std::unique_ptr<const llvm::MCSubtargetInfo> _sti;
  std::unique_ptr<const llvm::MCObjectFileInfo> _mofi;
  std::unique_ptr<llvm::MCContext> _mc;
  std::unique_ptr<const llvm::MCDisassembler> _disAsm;
  std::unique_ptr<const llvm::MCInstrInfo> _mii;
  std::unique_ptr<llvm::MCInstPrinter> _instPrinter;

  // icaller
  Function _iCaller;
  Map<std::string, FunctionPtr> _funMap;

  uint64_t _extFuncAddr = 0;
  std::unique_ptr<llvm::Module> _lcModule;

  // host functions

  FunctionPtr _getCycles;
  FunctionPtr _getTime;
  FunctionPtr _getInstRet;


  // methods

  llvm::Error start();
  llvm::Error finish();

  // indirect function caller
  llvm::Error genICaller();
};

} // sbt

#endif
