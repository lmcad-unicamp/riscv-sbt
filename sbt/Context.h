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
class Stack;
class Syscall;
class Translator;
class XRegister;
class XRegisters;

using FunctionPtr = Pointer<Function>;

class Context
{
public:
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

  llvm::LLVMContext* ctx;
  llvm::Module* module;
  llvm::IRBuilder<>* builder;
  Constants c;
  Types t;
  ArrayPtr<XRegisters, XRegister> x;
  Syscall* syscall = nullptr;
  Builder* bld = nullptr;
  Stack* stack = nullptr;
  llvm::GlobalVariable* shadowImage = nullptr;
  Disassembler* disasm = nullptr;
  SBTRelocation* reloc = nullptr;
  Translator* translator = nullptr;
  Map<std::string, FunctionPtr>* _func = nullptr;

  Map<std::string, FunctionPtr>& func()
  {
    return *_func;
  }
};

}

#endif
