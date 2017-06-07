#include "Relocation.h"

#include "Builder.h"
#include "Object.h"
#include "Translator.h"

#include <llvm/Object/ELF.h>
#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 0
#include "Debug.h"

namespace sbt {

SBTRelocation::SBTRelocation(
  Context* ctx,
  ConstRelocIter ri,
  ConstRelocIter re)
  :
  _ctx(ctx),
  _ri(ri),
  _re(re),
  _rlast(ri),
  _last(0, 0, "", nullptr, 0)  // dummy last symbol
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

  DBGS << __FUNCTION__ << llvm::formatv("({0:X+4})\n", addr);

  ConstSymbolPtr sym = reloc->symbol();
  uint64_t type = reloc->type();
  bool isLO = false;
  uint64_t mask;
  ConstSymbolPtr realSym = sym;

  switch (type) {
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_HI20:
      break;

    // this rellocation has PC info only
    // symbol info is present on the PCREL_HI20 reloc
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
      isLO = true;
      realSym = (**_rlast).symbol();
      break;

    case llvm::ELF::R_RISCV_LO12_I:
      isLO = true;
      break;

    default:
      DBGS << "relocation type: " << type << nl;
      xassert(false && "unknown relocation");
  }

  // set symbol relocation info
  SBTSymbol ssym(realSym->address(), realSym->address(),
    realSym->name(), realSym->section(), addr);
  DBGS << __FUNCTION__
    << llvm::formatv(": addr={0:X+4}, val={1:X+4}, name={2}\n",
          ssym.addr, ssym.val, ssym.name);

  xassert((ssym.sec || !ssym.addr) && "no section found for relocation");
  if (ssym.sec) {
    xassert(ssym.addr < ssym.sec->size() && "out of bounds relocation");
    ssym.val += ssym.sec->shadowOffs();
  }

  // increment relocation iterator until it reaches a different address
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
  // XXX using symbol info
  bool isFunction =
    ssym.sec && ssym.sec->isText() &&
    realSym->type() == llvm::object::SymbolRef::ST_Function;

  // external function case: return our handler instead
  if (ssym.isExternal()) {
    auto expAddr =_ctx->translator->import(ssym.name);
    if (!expAddr)
      return expAddr.takeError();
    uint64_t addr = expAddr.get();
    DBGF("external: addr={0:X+8}, mask={1:X+8}", addr, mask);
    addr &= mask;
    v = _ctx->c.i32(addr);

  // internal function case
  } else if (isFunction) {
    uint64_t addr = ssym.addr;
    addr &= mask;
    v = _ctx->c.i32(addr);

  // other relocation types:
  // add the relocation offset to ShadowImage to get the final address
  } else if (ssym.sec) {
    Builder* bld = _ctx->bld;

    // get char* to memory
    DBGS << __FUNCTION__ << llvm::formatv(": reloc={0:X+4}\n", ssym.val);
    // abort();
    std::vector<llvm::Value*> idx = { _ctx->c.ZERO, _ctx->c.i32(ssym.val) };
    v = bld->gep(_ctx->shadowImage, idx);

    v = bld->i8PtrToI32(v);

    // get only the upper or lower part of the result
    v = bld->_and(v, _ctx->c.i32(mask));

  } else
    xassert(false && "failed to resolve relocation");

  _last = std::move(ssym);
  _hasSymbol = true;
  return v;
}

}
