#ifndef SBT_SBTERROR_H
#define SBT_SBTERROR_H

#include "Constants.h"

#include <llvm/Support/Error.h>

#include <memory>
#include <string>

namespace sbt {

template <typename E>
class SBTErrorBase : public llvm::ErrorInfo<E>
{
public:
    /**
     * ctor.
     *
     * @param msg
     */
    SBTErrorBase(const std::string& msg = "")
        :
        _ss(new llvm::raw_string_ostream(_s)),
        _cause(llvm::Error::success())
    {
        // error format: <sbt>: error: prefix: <msg>
        *_ss << Constants::global().BIN_NAME << ": error: " << msg;
    }

    // dtor
    ~SBTErrorBase() override
    {
        consumeError(std::move(_cause));
    }

    // disallow copy
    SBTErrorBase(const SBTErrorBase&) = delete;

    // move
    SBTErrorBase(SBTErrorBase&& other)
        :
        _s(std::move(other._ss->str())),
        _ss(new llvm::raw_string_ostream(_s)),
        _cause(std::move(other._cause))
    {
    }

    // log error
    void log(llvm::raw_ostream& os) const override
    {
        os << _ss->str() << nl;
        if (_cause) {
            logAllUnhandledErrors(std::move(_cause), os, "cause: ");
            _cause = llvm::Error::success();
        }
    }

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

    SBTErrorBase& operator<<(llvm::Error&& err)
    {
        _cause = std::move(err);
        return *this;
    }

    // msg
    std::string msg() const
    {
        std::string s;
        llvm::raw_string_ostream ss(s);
        log(ss);
        return s;
    }

    // used by ErrorInfo::classID.
    static char ID;

private:
    std::string _s;                                 // error message
    std::unique_ptr<llvm::raw_string_ostream> _ss;  // string stream
    mutable llvm::Error _cause;
};


// convert SBTErrorBase to llvm::Error

template <typename E>
static inline llvm::Error error(SBTErrorBase<E>&& err)
{
    return llvm::make_error<SBTErrorBase<E>>(std::move(err));
}

template <typename E>
static inline llvm::Error error(SBTErrorBase<E>& err)
{
    return error(std::move(err));
}


// generic SBT error
class SBTError : public SBTErrorBase<SBTError>
{
public:
    using SBTErrorBase::SBTErrorBase;

    static char ID;
};


// translation generated invalid bitcode
class InvalidBitcode : public SBTErrorBase<InvalidBitcode>
{
public:
    using SBTErrorBase::SBTErrorBase;

    static char ID;
};


class InvalidInstructionEncoding :
    public SBTErrorBase<InvalidInstructionEncoding>
{
public:
    using SBTErrorBase::SBTErrorBase;

    static char ID;
};


class FunctionNotFound : public SBTErrorBase<FunctionNotFound>
{
public:
    using SBTErrorBase::SBTErrorBase;

    static char ID;
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


// xabort

[[noreturn]] static inline void xabort(
    const std::string& meth, const std::string& msg)
{
    llvm::errs() << meth + "(): " + msg << nl;
    std::exit(1);
}

#define XABORT(msg) xabort(__METHOD_NAME__, msg)
#define XABORTF(msg, ...) XABORT(llvm::formatv(msg, __VA_ARGS__))

} // sbt

#endif
