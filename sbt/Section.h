#ifndef SBT_SECTION_H
#define SBT_SECTION_H

#include "Object.h"

#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Module;
}

namespace sbt {

class SBTSection
{
public:
  SBTSection(
    ConstSectionPtr sec,
    llvm::Module* mod,
    llvm::IRBuilder<>* builder,
    llvm::LLVMContext* ctx)
    :
    _section(sec),
    _module(mod),
    _builder(builder),
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
  llvm::Module* _module;
  llvm::IRBuilder<>* _builder;
  llvm::LLVMContext* _ctx;

  llvm::ArrayRef<uint8_t> _sectionBytes;
  ConstRelocIter _ri;
  ConstRelocIter _re;
  ConstRelocIter _rlast;
};

}

#endif
