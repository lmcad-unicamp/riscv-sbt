#ifndef SBT_UTILS_H
#define SBT_UTILS_H

#include "Constants.h"

#include <llvm/Support/Error.h>

#include <functional>
#include <type_traits>
#include <vector>

// macros

#define UNUSED(x) (void)(x)

#define MIN(a,b) ((a) <= (b)? (a) : (b))
#define MAX(a,b) ((a) >= (b)? (a) : (b))


namespace sbt {

// object creation functions

// for pointers
template <typename T, typename... Args>
typename std::enable_if<
  std::is_pointer<T>::value,
  llvm::Expected<T>>::type
create(Args&&... args)
{
  llvm::Error err = llvm::Error::success();
  llvm::consumeError(std::move(err));

  typedef typename std::remove_pointer<T>::type TT;
  T ptr = new TT(args..., err);
  if (err) {
    delete ptr;
    return std::move(err);
  } else
    return ptr;
}

// for non-pointers
template <typename T, typename... Args>
typename std::enable_if<
  !std::is_pointer<T>::value,
  llvm::Expected<T>>::type
create(Args&&... args)
{
  llvm::Error err = llvm::Error::success();
  llvm::consumeError(std::move(err));

  T inst(args..., err);
  if (err)
    return std::move(err);
  return std::move(inst);
}

// execute some action when exiting scope
class OnScopeExit
{
public:
  using FnType = std::function<void()>;

  OnScopeExit(FnType&& fn) :
    _fn(std::move(fn))
  {}

  ~OnScopeExit()
  {
    _fn();
  }

private:
  FnType _fn;
};

// return an already consumed error
static inline llvm::Error noError()
{
  llvm::Error err = llvm::Error::success();
  llvm::consumeError(std::move(err));
  return err;
}

// helper to check if Expected has a value or an error
// value: set val
// error: set err
// return: true if no errors
template <typename T>
bool exp(llvm::Expected<T> exp, T& val, llvm::Error& err)
{
  if (!exp) {
    err = std::move(exp.takeError());
    return false;
  } else {
    val = std::move(exp.get());
    return true;
  }
}

} // sbt

#endif
