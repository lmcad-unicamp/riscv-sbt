#ifndef SBT_DISASSEMBLER_H
#define SBT_DISASSEMBLER_H

#include <llvm/Support/Error.h>

namespace llvm {
class MCDisassembler;
class MCInst;
class MCInstPrinter;
class MCSubtargetInfo;
}

namespace sbt {

class Disassembler
{
public:
  Disassembler(
    const llvm::MCDisassembler* da,
    llvm::MCInstPrinter* printer,
    const llvm::MCSubtargetInfo* sti)
    :
    _disasm(da),
    _printer(printer),
    _sti(sti)
  {}

  /**
   * Disassemble one instruction.
   *
   * @param addr instruction address
   * @param rawInst instruction in binary format
   * @param inst [output] disassembled instruction
   * @param size [output] instruction size
   */
  llvm::Error disasm(
    uint64_t addr,
    uint32_t rawInst,
    llvm::MCInst& inst,
    size_t& size);

private:
  const llvm::MCDisassembler* _disasm;
  llvm::MCInstPrinter* _printer;
  const llvm::MCSubtargetInfo* _sti;
};

}

#endif
