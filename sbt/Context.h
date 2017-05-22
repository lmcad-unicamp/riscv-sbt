#ifndef SBT_CONTEXT_H
#define SBT_CONTEXT_H

#include "Constants.h"
#include "Map.h"
#include "Pointer.h"
#include "Types.h"
#include "Utils.h"

#include <llvm/IR/IRBuilder.h>

namespace sbt {

class Builder;
class Disassembler;
class Function;
class SBTRelocation;
class SBTSection;
class Stack;
class Syscall;
class Translator;
class XRegister;
class XRegisters;

class Context
{
public:
  using FunctionPtr = Pointer<Function>;

  Context(
    llvm::LLVMContext* ctx,
    llvm::Module* mod,
    llvm::IRBuilder<>* bld)
    :
    ctx(ctx),
    module(mod),
    builder(bld),
    t(*ctx)
  {
    c.init(ctx);
  }

  ~Context();

  // program scope

  // llvm
  llvm::LLVMContext* ctx;
  llvm::Module* module;
  llvm::IRBuilder<>* builder;
  // translator
  Translator* translator = nullptr;
  // disassembler
  Disassembler* disasm = nullptr;
  // constants
  Constants c;
  // types
  Types t;
  // regs
  ArrayPtr<XRegisters, XRegister> x;
  // syscall handler
  Syscall* syscall = nullptr;
  // stack
  Stack* stack = nullptr;
  // flags
  bool inMain = false;
  // function maps
  Map<std::string, FunctionPtr>* _func = nullptr;
  Map<uint64_t, Function*>* _funcByAddr = nullptr;

  Map<std::string, FunctionPtr>& func()
  {
    return *_func;
  }

  Map<uint64_t, Function*>& funcByAddr()
  {
    return *_funcByAddr;
  }

  // module scope

  // object
  // ConstObjectPtr obj = nullptr;
  // shadow image
  llvm::GlobalVariable* shadowImage = nullptr;

  // section scope

  // section
  SBTSection* sec = nullptr;
  // relocator
  SBTRelocation* reloc = nullptr;
  // builder
  Builder* bld = nullptr;

  // function scope

  // flags
  bool brWasLast = false;
};

}

#endif
