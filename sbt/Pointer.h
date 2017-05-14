#ifndef SBT_POINTER_H
#define SBT_POINTER_H

#include "Utils.h"

#include <memory>

namespace sbt {

#define NULL_CHECK xassert(_ptr && "null pointer")

template <typename T>
class Pointer
{
public:
  explicit Pointer(T* ptr = nullptr) :
    _ptr(ptr)
  {}

  T* get()
  {
    return _ptr.get();
  }

  const T* get() const
  {
    return _ptr.get();
  }

  void reset(T* ptr)
  {
    _ptr.reset(ptr);
  }

  T* operator->()
  {
    NULL_CHECK;
    return get();
  }

  const T* operator->() const
  {
    NULL_CHECK;
    return get();
  }

  T& operator*()
  {
    NULL_CHECK;
    return *get();
  }

  const T& operator*() const
  {
    NULL_CHECK;
    return *get();
  }

  bool operator==(const Pointer& other) const
  {
    return get() == other.get();
  }

  bool operator<(const Pointer& other) const
  {
    return get() < other.get();
  }

private:
  std::unique_ptr<T> _ptr;
};

}

#endif
