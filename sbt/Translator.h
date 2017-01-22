#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Object.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Error.h>

namespace llvm {
class LLVMContext;
class Function;
class FunctionType;
class IntegerType;
class GlobalVariable;
class MCInst;
class Module;
class Value;
}

namespace sbt {

class Translator
{
public:
  Translator(
    llvm::LLVMContext *Ctx,
    llvm::IRBuilder<> *Bldr,
    llvm::Module *Mod);

  ~Translator();

  // translate one instruction
  llvm::Error translate(const llvm::MCInst &Inst);

  llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);

  void setCurAddr(uint64_t Addr)
  {
    CurAddr = Addr;
  }

  void setCurObj(ConstObjectPtr Obj)
  {
    CurObj = Obj;
  }

  void setCurSection(ConstSectionPtr Section)
  {
    CurSection = Section;
  }

private:
  // constants
  llvm::IntegerType *I32;
  llvm::Value *ZERO;
  // data
  llvm::LLVMContext *Context;
  llvm::IRBuilder<> *Builder;
  llvm::Module *Module;
  bool FirstFunction = true;
  llvm::GlobalVariable *X[32];
  llvm::FunctionType *FTSyscall4;
  llvm::Function *FSyscall4;

  uint64_t CurAddr;
  ConstObjectPtr CurObj = nullptr;
  ConstSectionPtr CurSection;

  // methods
  llvm::Value *load(unsigned Reg);
  void store(llvm::Value *V, unsigned Reg);

  void buildRegisterFile();

  // llvm::Expected<llvm::Value *> resolveRelocation();
};

} // sbt

#endif
