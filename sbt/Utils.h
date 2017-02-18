#ifndef SBT_UTILS_H
#define SBT_UTILS_H

#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <type_traits>
#include <vector>

namespace llvm {
namespace object {
class ObjectFile;
class SectionRef;
}
}

#define DBGS_ON

#define UNUSED(x) (void)(x)

#define MIN(a,b) ((a) <= (b)? (a) : (b))
#define MAX(a,b) ((a) >= (b)? (a) : (b))

namespace sbt {

llvm::raw_ostream &logs(bool error = false);

#ifdef DBGS_ON
static llvm::raw_ostream &DBGS = llvm::outs();
#else
static llvm::raw_ostream &DBGS = llvm::nulls();
#endif


template <typename T, typename... Args>
typename std::enable_if<
  std::is_pointer<T>::value,
  llvm::Expected<T>>::type
create(Args&&... args)
{
  llvm::Error E = llvm::Error::success();
  llvm::consumeError(std::move(E));

  typedef typename std::remove_pointer<T>::type TT;
  T Ptr = new TT(args..., E);
  if (E) {
    delete Ptr;
    return std::move(E);
  } else
    return Ptr;
}

template <typename T, typename... Args>
typename std::enable_if<
  !std::is_pointer<T>::value,
  llvm::Expected<T>>::type
create(Args&&... args)
{
  llvm::Error E = llvm::Error::success();
  llvm::consumeError(std::move(E));

  T Inst(args..., E);
  if (E)
    return std::move(E);
  return std::move(Inst);
}

} // sbt

#endif
