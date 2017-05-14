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

private:
  using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

  enum TranslationState {
    ST_DFL,
    ST_PADDING
  };

  TranslationState _state = ST_DFL;

  ConstSectionPtr _section;
  Context* _ctx;

  llvm::ArrayRef<uint8_t> _sectionBytes;
  ConstRelocIter _ri;
  ConstRelocIter _re;
  ConstRelocIter _rlast;
};

}

#endif
