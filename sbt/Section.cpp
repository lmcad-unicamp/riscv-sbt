#include "Section.h"

#include "Builder.h"
#include "Function.h"
#include "Relocation.h"
#include "SBTError.h"

#include <llvm/Support/FormatVariadic.h>

namespace sbt {

llvm::Error SBTSection::translate()
{
  // skip non code sections
  if (!_section->isText())
    return llvm::Error::success();

  uint64_t addr = _section->address();
  uint64_t size = _section->size();

  // relocatable object?
  uint64_t elfOffset = addr;
  if (addr == 0)
    elfOffset = _section->getELFOffset();

  // print section info
  DBGS << llvm::formatv("Section {0}: Addr={1:X+4}, ELFOffs={2:X+4}, Size={3:X+4}\n",
      _section->name(), addr, elfOffset, size);

  // get relocations
  const ConstRelocationPtrVec relocs = _section->object()->relocs();
  SBTRelocation reloc(_ctx, relocs.cbegin(), relocs.cend());
  _ctx->reloc = &reloc;

  Builder bld(_ctx);
  _ctx->bld = &bld;
  _ctx->sec = this;

  // get section bytes
  llvm::StringRef bytesStr;
  if (_section->contents(bytesStr)) {
    SBTError serr;
    serr << "Failed to get Section Contents";
    return error(serr);
  }
  _bytes = llvm::ArrayRef<uint8_t>(
    reinterpret_cast<const uint8_t *>(bytesStr.data()), bytesStr.size());

  // for each symbol
  const ConstSymbolPtrVec &symbols = _section->symbols();
  size_t n = symbols.size();
  for (size_t i = 0; i < n; ++i) {
    ConstSymbolPtr sym = symbols[i];

    uint64_t symaddr = sym->address();
    volatile uint64_t end;  // XXX gcc bug: need to make it volatile
    if (i == n - 1)
      end = size;
    else
      end = symbols[i + 1]->address();

    // XXX for now, translate only instructions that appear after a
    // function or global symbol
    const llvm::StringRef symname = sym->name();
    if (sym->type() == llvm::object::SymbolRef::ST_Function ||
        sym->flags() & llvm::object::SymbolRef::SF_Global)
    {
      // if (symname == "main")
      Function* f = new Function(_ctx, symname, this, symaddr, end);
      FunctionPtr func(f);
      // _ctx->func()(std::move(func), std::move(symaddr));
      if (auto err = f->translate())
        return err;
    // skip section bytes until a function like symbol is found
    } else
      continue;
  }

  return llvm::Error::success();
}

}
