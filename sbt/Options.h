#ifndef SBT_OPTIONS_H
#define SBT_OPTIONS_H

#include <string>

#include <cstdint>

namespace sbt {

class Options
{
public:
    enum class Regs {
        GLOBALS,
        LOCALS,
        ABI
    };

    Options(
        Regs regs = Regs::GLOBALS,
        bool useLibC = true,
        std::size_t stackSize = 0)
        : _regs(regs),
          _useLibC(useLibC),
          _stackSize(stackSize)
    {
    }

    Regs regs() const
    {
        return _regs;
    }

    bool useLibC() const
    {
        return _useLibC;
    }

    std::size_t stackSize() const
    {
        return _stackSize;
    }

    bool syncFRegs() const
    {
        return _syncFRegs;
    }

    Options& setSyncFRegs(bool b)
    {
        _syncFRegs = b;
        return *this;
    }

    const std::string& a2s() const
    {
        return _a2s;
    }

    Options& setA2S(const std::string& file)
    {
        _a2s = file;
        return *this;
    }

    bool syncOnExternalCalls() const
    {
        return _syncOnExternalCalls;
    }

    Options& setSyncOnExternalCalls(bool b)
    {
        _syncOnExternalCalls = b;
        return *this;
    }

    bool commentedAsm() const
    {
        return _commentedAsm;
    }

    Options& setCommentedAsm(bool b)
    {
        _commentedAsm = b;
        return *this;
    }

    bool symBoundsCheck() const
    {
        return _symBoundsCheck;
    }

    Options& setSymBoundsCheck(bool b)
    {
        _symBoundsCheck = b;
        return *this;
    }

    bool enableFCSR() const
    {
        return _enableFCSR;
    }

    Options& setEnableFCSR(bool b)
    {
        _enableFCSR = b;
        return *this;
    }

    bool enableFCVTValidation() const
    {
        return _enableFCVTValidation;
    }

    Options& setEnableFCVTValidation(bool b)
    {
        _enableFCVTValidation = b;
        return *this;
    }

    // use ICaller for Indirect Internal Functions
    bool useICallerForIIntFuncs() const
    {
        return false;
    }

    bool hardFloatABI() const
    {
        return _hf;
    }

    Options& setHardFloatABI(bool b)
    {
        _hf = b;
        return *this;
    }

    bool optStack() const
    {
        return _optStack;
    }

    Options& setOptStack(bool b)
    {
        _optStack = b;
        return *this;
    }

    bool icallIntOnly() const
    {
        return _icallIntOnly;
    }

    Options& setICallIntOnly(bool b)
    {
        _icallIntOnly = b;
        return *this;
    }

    const std::string& logFile() const
    {
        return _logFile;
    }

    Options& setLogFile(const std::string& path)
    {
        _logFile = path;
        return *this;
    }

    void dump() const;

private:
    Regs _regs;
    bool _useLibC;
    std::size_t _stackSize;
    bool _syncFRegs = true;
    std::string _a2s;
    bool _syncOnExternalCalls = false;
    bool _commentedAsm = false;
    bool _symBoundsCheck = true;
    bool _enableFCSR = false;
    bool _enableFCVTValidation = false;
    bool _hf = true;
    bool _optStack = false;
    bool _icallIntOnly = false;
    std::string _logFile;
};

}

#endif
