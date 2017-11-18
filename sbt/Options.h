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

    Options(Regs regs = Regs::GLOBALS)
        : _regs(regs)
    {
    }

    Regs regs() const
    {
        return _regs;
    }

private:
    Regs _regs;
};

}

#endif
