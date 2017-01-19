#ifndef SBT_SBTERROR_H
#define SBT_SBTERROR_H

#include <llvm/Support/Error.h>

#include <memory>
#include <string>

namespace sbt {

// Our custom error class
class SBTError : public llvm::ErrorInfo<SBTError>
{
public:
  // ctor
  // FileName - name of the input file that was being processed
  //            when the error happened
  SBTError(const std::string &FileName = "");

  // disallow copy
  SBTError(const SBTError &) = delete;

  // move
  SBTError(SBTError &&X);

  // log error
  void log(llvm::raw_ostream &OS) const override;

  // unused error_code conversion compatibility method
  std::error_code convertToErrorCode() const override
  {
    return llvm::inconvertibleErrorCode();
  }

  // stream insertion overloads to make it easy
  // to build the error message
  template <typename T>
  llvm::raw_string_ostream &operator<<(const T &&Val)
  {
    *SS << Val;
    return *SS;
  }

  llvm::raw_string_ostream &operator<<(const char *Val)
  {
    *SS << Val;
    return *SS;
  }

  SBTError &operator<<(llvm::Error &&E)
  {
    Cause = std::move(E);
    return *this;
  }

  // Used by ErrorInfo::classID.
  static char ID;

private:
  std::string S;                                  // error message
  std::unique_ptr<llvm::raw_string_ostream> SS;   // string stream
  mutable llvm::Error Cause;
};

static inline llvm::Error error(SBTError &&SE)
{
  return llvm::make_error<SBTError>(std::move(SE));
}

static inline llvm::Error error(SBTError &SE)
{
  return error(std::move(SE));
}

} // sbt

#endif
