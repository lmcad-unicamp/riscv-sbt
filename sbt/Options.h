#ifndef SBT_OPTIONS_H
#define SBT_OPTIONS_H

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
        bool useLibC = true)
        : _regs(regs),
          _useLibC(useLibC)
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

    void dump() const;

private:
    Regs _regs;
    bool _useLibC;
};

}

#endif
