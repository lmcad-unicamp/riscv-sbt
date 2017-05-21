#include "Relocation.h"

#include "Builder.h"
#include "Object.h"
#include "Symbol.h"
#include "Translator.h"

#include <llvm/Object/ELF.h>

namespace sbt {

SBTRelocation::SBTRelocation(
  Context* ctx,
  ConstRelocIter ri,
  ConstRelocIter re)
  :
  _ctx(ctx),
  _ri(ri),
  _re(re),
  _rlast(ri)
{
}


llvm::Expected<llvm::Value*>
SBTRelocation::handleRelocation(uint64_t addr, llvm::raw_ostream* os)
{
  // no more relocations exist
  if (_ri == _re)
    return nullptr;

  // check if there is a relocation for current address
  auto reloc = *_ri;
  if (reloc->offset() != addr)
    return nullptr;

  ConstSymbolPtr sym = reloc->symbol();
  uint64_t type = reloc->type();
  bool isLO = false;
  uint64_t mask;
  ConstSymbolPtr realSym = sym;

  switch (type) {
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_HI20:
      break;

    // This rellocation has PC info only
    // Symbol info is present on the PCREL_HI20 Reloc
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
      isLO = true;
      realSym = (**_rlast).symbol();
      break;

    case llvm::ELF::R_RISCV_LO12_I:
      isLO = true;
      break;

    default:
      DBGS << "Relocation Type: " << type << nl;
      xassert(false && "unknown relocation");
  }

  // set Symbol Relocation info
  SBTSymbol ssym(realSym->address(), realSym->address(),
    realSym->name(), realSym->section());

  // Note: !SR.Sec && SR.Addr == External Symbol
  xassert((ssym.sec || !ssym.addr) && "No section found for relocation");
  if (ssym.sec) {
    xassert(ssym.addr < ssym.sec->size() && "Out of bounds relocation");
    ssym.val += ssym.sec->shadowOffs();
  }

  // increment relocation iterator
  _rlast = _ri;
  do {
    ++_ri;
  } while (_ri != _re && (**_ri).offset() == addr);

  // write relocation string
  if (isLO) {
    mask = 0xFFF;
    *os << "%lo(";
  } else {
    mask = 0xFFFFF000;
    *os << "%hi(";
  }
  *os << ssym.name << ") = " << ssym.addr;

  llvm::Value* v = nullptr;
  bool isFunction =
    ssym.sec && ssym.sec->isText() &&
    realSym->type() == llvm::object::SymbolRef::ST_Function;

  // special case: external functions: return our handler instead
  if (ssym.isExternal()) {
    auto expAddr =_ctx->translator->import(ssym.name);
    if (!expAddr)
      return expAddr.takeError();
    uint64_t addr = expAddr.get();
    addr &= mask;
    v = _ctx->c.i32(addr);

  // special case: internal function
  } else if (isFunction) {
    uint64_t addr = ssym.addr;
    addr &= mask;
    v = _ctx->c.i32(addr);

  // add the relocation offset to ShadowImage to get the final address
  } else if (ssym.sec) {
    Builder* bld = _ctx->bld;

    // get char* to memory
    std::vector<llvm::Value*> idx = { _ctx->c.ZERO, _ctx->c.i32(ssym.val) };
    v = bld->gep(_ctx->shadowImage, idx);

    v = bld->i8PtrToI32(v);

    // Finally, get only the upper or lower part of the result
    v = bld->_and(v, _ctx->c.i32(mask));

  } else
    xassert(false && "Failed to resolve relocation");

  return v;
}

}
