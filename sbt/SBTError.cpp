#include "SBTError.h"
#include "Constants.h"

namespace sbt {

char SBTError::ID = 'S';

SBTError::SBTError(const std::string &FileName) :
  SS(new llvm::raw_string_ostream(S))
{
  // error format: <sbt>: error: '<file>': <msg>
  *SS << *BIN_NAME << ": error: ";
  if (!FileName.empty())
    *SS << "'" << FileName << "': ";
}

SBTError::SBTError(SBTError &&X) :
  S(std::move(X.SS->str())),
  SS(new llvm::raw_string_ostream(S))
{
}

void SBTError::log(llvm::raw_ostream &OS) const
{
  OS << SS->str();
}

} // sbt
