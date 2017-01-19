#include "SBTError.h"
#include "Constants.h"

using namespace llvm;

namespace sbt {

char SBTError::ID = 'S';

SBTError::SBTError(const std::string &FileName) :
  SS(new raw_string_ostream(S)),
  Cause(Error::success())
{
  // error format: <sbt>: error: '<file>': <msg>
  *SS << *BIN_NAME << ": error: ";
  if (!FileName.empty())
    *SS << "'" << FileName << "': ";
}

SBTError::SBTError(SBTError &&X) :
  S(std::move(X.SS->str())),
  SS(new llvm::raw_string_ostream(S)),
  Cause(std::move(X.Cause))
{
}

SBTError::~SBTError()
{
  consumeError(std::move(Cause));
}

void SBTError::log(llvm::raw_ostream &OS) const
{
  OS << SS->str();
  if (Cause) {
    logAllUnhandledErrors(std::move(Cause), OS, "Cause: ");
    Cause = Error::success();
  }
}

} // sbt
