#include "Utils.h"

#include "Constants.h"

#include <llvm/Support/raw_ostream.h>

namespace sbt {

llvm::raw_ostream& logs(bool error)
{
  return (error? llvm::errs() : llvm::outs()) << *BIN_NAME << ": ";
}

}
