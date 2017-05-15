#ifndef SBT_SECTION_H
#define SBT_SECTION_H

#include "Context.h"
#include "Object.h"

namespace sbt {

class SBTSection
{
public:
  SBTSection(
    ConstSectionPtr sec,
    Context* ctx)
    :
    _section(sec),
    _ctx(ctx)
  {}

  llvm::Error translate();

  ConstSectionPtr section() const
  {
    return _section;
  }

  const llvm::ArrayRef<uint8_t> bytes() const
  {
    return _bytes;
  }

private:
  using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

  ConstSectionPtr _section;
  Context* _ctx;

  llvm::ArrayRef<uint8_t> _bytes;
  ConstRelocIter _ri;
  ConstRelocIter _re;
  ConstRelocIter _rlast;
};

}

#endif
