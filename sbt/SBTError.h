#ifndef SBT_SBTERROR_H
#define SBT_SBTERROR_H

#include <llvm/Support/Error.h>

#include <memory>
#include <string>

namespace sbt {

// custom error class
class SBTError : public llvm::ErrorInfo<SBTError>
{
public:
    // ctor
    // fileName - name of the input file that was being processed
    //                        when the error happened
    SBTError(const std::string& fileName = "");

    // disallow copy
    SBTError(const SBTError&) = delete;
    // move
    SBTError(SBTError&&);
    // dtor
    ~SBTError() override;

    // log error
    void log(llvm::raw_ostream& os) const override;

    // unused error_code conversion compatibility method
    std::error_code convertToErrorCode() const override
    {
        return llvm::inconvertibleErrorCode();
    }

    // stream insertion overloads to make it easy
    // to build the error message
    template <typename T>
    llvm::raw_string_ostream& operator<<(const T&& val)
    {
        *_ss << val;
        return *_ss;
    }

    llvm::raw_string_ostream& operator<<(const char* val)
    {
        *_ss << val;
        return *_ss;
    }

    SBTError& operator<<(llvm::Error&& err)
    {
        _cause = std::move(err);
        return *this;
    }

    std::string msg() const;

    // used by ErrorInfo::classID.
    static char ID;

private:
    std::string _s;                                 // error message
    std::unique_ptr<llvm::raw_string_ostream> _ss;  // string stream
    mutable llvm::Error _cause;
};

// convert SBTError to llvm::Error

static inline llvm::Error error(SBTError&& err)
{
    return llvm::make_error<SBTError>(std::move(err));
}

static inline llvm::Error error(SBTError& err)
{
    return error(std::move(err));
}


// translation generated invalid bitcode
class InvalidBitcode : public SBTError
{
public:
    using SBTError::SBTError;
};


class InvalidInstructionEncoding : public SBTError
{
public:
    using SBTError::SBTError;
};


class FunctionNotFound : public SBTError
{
public:
    using SBTError::SBTError;
};

} // sbt

#endif
