#include "Disassembler.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/Support/FormatVariadic.h>

namespace sbt {

llvm::Error Disassembler::disasm(
  uint64_t addr,
  uint32_t rawInst,
  llvm::MCInst& inst,
  size_t& size)
{
  llvm::MCDisassembler::DecodeStatus sts =
    _disasm->getInstruction(inst, size,
      llvm::ArrayRef<uint8_t>(
        reinterpret_cast<uint8_t*>(&rawInst), sizeof(rawInst)),
      addr, DBGS, llvm::nulls());
  if (sts == llvm::MCDisassembler::DecodeStatus::Success) {
#if SBT_DEBUG
    // print instruction
    DBGS << llvm::formatv("{0:X-4}: ", addr);
    _printer->printInst(&inst, DBGS, "", *_sti);
    DBGS << "\n";
#endif
  // failed to disasm
  } else {
    SBTError serr;
    serr << "invalid instruction encoding at address ";
    serr << llvm::formatv("{0:X-4}", addr);
    serr << llvm::formatv(": {0:X-8}", rawInst);
    return error(serr);
  }

  return llvm::Error::success();
}

}
