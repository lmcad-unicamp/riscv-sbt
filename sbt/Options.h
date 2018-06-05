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
        LOCALS
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

    void setSymBoundsCheck(bool b)
    {
        _symBoundsCheck = b;
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
};

}

#endif
