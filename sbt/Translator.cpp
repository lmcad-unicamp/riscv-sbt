#include "Translator.h"

#include "Builder.h"
#include "Module.h"
#include "Register.h"
#include "SBTError.h"
#include "Syscall.h"
#include "Utils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>

namespace llvm {
Target& getTheRISCVMaster32Target();
}


namespace sbt {

Translator::Translator(Context* ctx)
  : _ctx(ctx)
{
  initLLVMConstants(*_ctx->ctx);
}


Translator::~Translator()
{
}


llvm::Error Translator::start()
{
  SBTError serr;

  // get target
  llvm::Triple triple("riscv32-unknown-elf");
  std::string tripleName = triple.getTriple();
  // logs() << "Triple: " << TripleName << "\n";

  _target = &llvm::getTheRISCVMaster32Target();
  // Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!_target) {
    serr << "Target not found: " << tripleName;
    return error(serr);
  }

  _mri.reset(_target->createMCRegInfo(tripleName));
  if (!_mri) {
    serr << "No register info for target " << tripleName;
    return error(serr);
  }

  // Set up disassembler.
  _asmInfo.reset(_target->createMCAsmInfo(*_mri, tripleName));
  if (!_asmInfo) {
    serr << "No assembly info for target " << tripleName;
    return error(serr);
  }

  llvm::SubtargetFeatures features;
  _sti.reset(
      _target->createMCSubtargetInfo(tripleName, "", features.getString()));
  if (!_sti) {
    serr << "No subtarget info for target " << tripleName;
    return error(serr);
  }

  _mofi.reset(new llvm::MCObjectFileInfo);
  _mc.reset(new llvm::MCContext(_asmInfo.get(), _mri.get(), _mofi.get()));
  _disAsm.reset(_target->createMCDisassembler(*_sti, *_mc));
  if (!_disAsm) {
    serr << "No disassembler for target " << tripleName;
    return error(serr);
  }

  _mii.reset(_target->createMCInstrInfo());
  if (!_mii) {
    serr << "No instruction info for target " << tripleName;
    return error(serr);
  }

  _instPrinter.reset(
    _target->createMCInstPrinter(triple, 0, *_asmInfo, *_mii, *_mri));
  if (!_instPrinter) {
    serr << "No instruction printer for target " << tripleName;
    return error(serr);
  }

  //
  //
  //
  if (auto err = declRegisterFile())
    return err;

  if (auto err = buildStack())
    return err;

  return llvm::Error::success();
}


llvm::Error Translator::declOrBuildRegisterFile(bool decl)
{
  Register::rvX[0] = new llvm::GlobalVariable(*_ctx->module, I32, CONSTANT,
    llvm::GlobalValue::ExternalLinkage, decl? nullptr : ZERO,
    Register::getXRegName() + "0");

  for (int i = 1; i < 32; ++i) {
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << Register::getXRegName() << i;
    Register::rvX[i] = new llvm::GlobalVariable(*_ctx->module, I32, !CONSTANT,
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

  llvm::Constant *cda = llvm::ConstantDataArray::get(*_ctx->ctx, byteArray);

  _stack = new llvm::GlobalVariable(
    *_ctx->module, cda->getType(), !CONSTANT,
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

  if (auto err = Syscall(_ctx).genHandler())
    return err;

  return llvm::Error::success();
}


llvm::Error Translator::translate()
{
  if (auto err = start())
    return err;

  for (const auto& f : _inputFiles) {
    Module mod(_ctx);
    Syscall(_ctx).declHandler();
    if (auto err = mod.translate(f))
      return err;
  }

  if (auto err = finish())
    return err;

  return llvm::Error::success();
}

} // sbt
