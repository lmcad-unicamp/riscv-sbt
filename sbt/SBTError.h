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
    /**
     * ctor.
     *
     * @param msg
     */
    SBTError(const std::string& msg = "");

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
    llvm::raw_string_ostream& operator<<(const T& val)
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

// verbose error

template <typename E>
static inline E vserror(const std::string& meth, const std::string& msg)
{
    return E(meth + "(): " + msg);
}

template <typename E>
static inline llvm::Error verror(const std::string& meth, const std::string& msg)
{
    return error(vserror<E>(meth, msg));
}

#define SERROR(msg) vserror<SBTError>(__METHOD_NAME__, msg)
#define ERROR2(E, msg) verror<E>(__METHOD_NAME__, msg)
#define ERROR(msg) ERROR2(SBTError, msg)

#define ERROR2F(E, msg, ...) ERROR2(E, llvm::formatv(msg, __VA_ARGS__))
#define ERRORF(msg, ...) ERROR2F(SBTError, msg, __VA_ARGS__)

} // sbt

#endif
