#ifndef SBT_SECTION_H
#define SBT_SECTION_H

#include "Context.h"
#include "Object.h"

namespace sbt {

class SBTSection
{
public:
  SBTSection(
    Context* ctx,
    ConstSectionPtr sec)
    :
    _ctx(ctx),
    _section(sec)
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
  Context* _ctx;
  ConstSectionPtr _section;

  llvm::ArrayRef<uint8_t> _bytes;

  struct Func {
    std::string name;
    uint64_t start;
    uint64_t end;
  };

  llvm::Error translate(const Func& func);
};

}

#endif
