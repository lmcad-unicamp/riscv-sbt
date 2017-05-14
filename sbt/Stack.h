#ifndef SBT_STACK_H
#define SBT_STACK_H

#include <cstdint>

namespace llvm {
class GlobalVariable;
class Value;
}

namespace sbt {

class Context;

class Stack
{
public:
  static const std::size_t SIZE = 4096;

  Stack(Context* ctx, std::size_t sz = SIZE);

  llvm::GlobalVariable* var() const
  {
    return _stack;
  }

  llvm::Value* end() const
  {
    return _end;
  }

private:
  std::size_t _size;

  llvm::GlobalVariable* _stack;
  llvm::Value* _end;
};

}

#endif
