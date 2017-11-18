#include "SBTError.h"

#include "Constants.h"

namespace sbt {

char SBTError::ID = 'S';


SBTError::SBTError(const std::string& prefix) :
  _ss(new llvm::raw_string_ostream(_s)),
  _cause(llvm::Error::success())
{
  // error format: <sbt>: error: prefix: <msg>
  *_ss << Constants::global().BIN_NAME << ": error: ";
  if (!prefix.empty())
    *_ss << prefix << ": ";
}


SBTError::SBTError(SBTError&& other) :
  _s(std::move(other._ss->str())),
  _ss(new llvm::raw_string_ostream(_s)),
  _cause(std::move(other._cause))
{
}


SBTError::~SBTError()
{
  consumeError(std::move(_cause));
}


void SBTError::log(llvm::raw_ostream& os) const
{
  os << _ss->str() << nl;
  if (_cause) {
    logAllUnhandledErrors(std::move(_cause), os, "cause: ");
    _cause = llvm::Error::success();
  }
}


std::string SBTError::msg() const
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    log(ss);
    return s;
}

} // sbt
