#ifndef SBT_OPTIONS_H
#define SBT_OPTIONS_H

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

    void dump() const;

private:
    Regs _regs;
    bool _useLibC;
    std::size_t _stackSize;
};

}

#endif
