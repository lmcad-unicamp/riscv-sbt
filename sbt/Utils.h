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

namespace sbt {

llvm::raw_ostream &logs(bool error = false);

#ifndef NDEBUG
static llvm::raw_ostream &DBGS = llvm::outs();
#else
static llvm::raw_ostream &DBGS = llvm::nulls();
#endif

/*
typedef std::vector<std::pair<uint64_t, llvm::StringRef>> SymbolVec;

// get all symbols present in Obj that belong to this Section
llvm::Expected<SymbolVec> getSymbolsList(
  const llvm::object::ObjectFile *Obj,
  const llvm::object::SectionRef &Section);
*/


template <typename T, typename... Args>
typename std::enable_if<
  std::is_pointer<T>::value,
  llvm::Expected<T>>::type
create(Args &... args)
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
create(Args &... args)
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
