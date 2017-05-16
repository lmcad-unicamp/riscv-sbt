#ifndef SBT_CONTEXT_H
#define SBT_CONTEXT_H

#include "Constants.h"
#include "Map.h"
#include "Pointer.h"
#include "Types.h"
#include "Utils.h"

#include <llvm/IR/IRBuilder.h>

namespace sbt {

class Disassembler;
class Function;
class Stack;
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
  Stack* stack = nullptr;
  Disassembler* disasm = nullptr;
  Map<FunctionPtr, uint64_t>* _func = nullptr;

  Map<FunctionPtr, uint64_t>& func()
  {
    return *_func;
  }
};

}

#endif
