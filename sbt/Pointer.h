#ifndef SBT_POINTER_H
#define SBT_POINTER_H

#include "Utils.h"

#include <memory>

namespace sbt {

#define NULL_CHECK xassert(_ptr && "null pointer")

/**
 * std::unique_ptr with more features.
 */
template <typename T>
class Pointer
{
public:
  explicit Pointer(T* ptr = nullptr) :
    _ptr(ptr)
  {}

  Pointer(Pointer&&) = default;
  Pointer& operator=(Pointer&&) = default;

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


/**
 * Wrapper to pointer to array, to be able to use []s without having to first
 * derreference the pointer.
 * ((*ptr)[i] -> aptr[i])
 */
template <typename A, typename E>
class ArrayPtr
{
public:
  ArrayPtr(A* ptr = nullptr) :
    _ptr(ptr)
  {}

  ArrayPtr& operator=(A* ptr)
  {
    _ptr = ptr;
    return *this;
  }

  const E& operator[](size_t p) const
  {
    xassert(_ptr);
    return (*_ptr)[p];
  }

  A* get()
  {
    return _ptr;
  }

private:
  A* _ptr = nullptr;
};

}

#endif
