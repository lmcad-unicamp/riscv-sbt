#include "Translator.h"

#include "Builder.h"
#include "Module.h"
#include "Register.h"
#include "SBTError.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Type.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/MemoryBuffer.h>

#include <algorithm>
#include <vector>

namespace sbt {

Translator::Translator(
  llvm::LLVMContext* ctx,
  llvm::IRBuilder<>* bldr,
  llvm::Module* mod)
  :
  _ctx(ctx),
  _builder(bldr),
  _module(mod)
{
  initLLVMConstants(*_ctx);
}


llvm::Error Translator::start()
{
  if (auto err = declRegisterFile())
    return err;

  if (auto err = buildStack())
    return err;

  _sc.declHandler(_module);
  return llvm::Error::success();
}


llvm::Error Translator::declOrBuildRegisterFile(bool decl)
{
  Register::rvX[0] = new llvm::GlobalVariable(*_module, I32, CONSTANT,
    llvm::GlobalValue::ExternalLinkage, decl? nullptr : ZERO,
    Register::getXRegName() + "0");

  for (int i = 1; i < 32; ++i) {
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << Register::getXRegName() << i;
    Register::rvX[i] = new llvm::GlobalVariable(*_module, I32, !CONSTANT,
        llvm::GlobalValue::ExternalLinkage, decl? nullptr : ZERO, ss.str());
  }

  return llvm::Error::success();
}


llvm::Error Translator::buildStack()
{
  std::string bytes(_stackSize, 'S');

  llvm::ArrayRef<uint8_t> byteArray(
    reinterpret_cast<const uint8_t *>(bytes.data()),
    _stackSize);

  llvm::Constant *cda = llvm::ConstantDataArray::get(*_ctx, byteArray);

  _stack = new llvm::GlobalVariable(
    *_module, cda->getType(), !CONSTANT,
      llvm::GlobalValue::ExternalLinkage, cda, "Stack");

  return llvm::Error::success();
}


llvm::Error Translator::finish()
{
  return llvm::Error::success();
}


llvm::Error Translator::genSCHandler()
{
  if (auto err = buildRegisterFile())
    return err;

  if (auto err = _sc.genHandler(_ctx, _builder, _module))
    return err;

  return llvm::Error::success();
}


llvm::Error Translator::translate()
{
  if (auto err = start())
    return err;

  for (const auto& f : _inputFiles) {
    Module mod(_ctx, _builder, _module);
    if (auto err = mod.translate(f))
      return err;
  }

  if (auto err = finish())
    return err;

  return llvm::Error::success();
}

} // sbt
