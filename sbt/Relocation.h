#ifndef SBT_RELOCATION_H
#define SBT_RELOCATION_H

#include "Context.h"
#include "Object.h"
#include "Symbol.h"

#include <llvm/IR/Value.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

namespace sbt {

class SBTRelocation
{
public:
  using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

  SBTRelocation(
    Context* ctx,
    ConstRelocIter ri,
    ConstRelocIter re);

  llvm::Expected<llvm::Value*>
  handleRelocation(uint64_t addr, llvm::raw_ostream* os);

  const SBTSymbol& last() const
  {
    return _last;
  }

  bool isSymbol(uint64_t addr) const
  {
    return _hasSymbol && _last.addr == addr;
  }

private:
  Context* _ctx;
  ConstRelocIter _ri;
  ConstRelocIter _re;
  ConstRelocIter _rlast;
  SBTSymbol _last;
  bool _hasSymbol = false;
};

}

#endif
