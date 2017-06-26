#ifndef SBT_SECTION_H
#define SBT_SECTION_H

#include "Context.h"
#include "Object.h"

namespace sbt {

class SBTSection
{
public:
    SBTSection(
        Context* ctx,
        ConstSectionPtr sec)
        :
        _ctx(ctx),
        _section(sec)
    {}

    llvm::Error translate();

    ConstSectionPtr section() const
    {
        return _section;
    }

    const llvm::ArrayRef<uint8_t> bytes() const
    {
        return _bytes;
    }

    uint64_t getNextFuncAddr() const
    {
        return _nextFuncAddr;
    }

    void updateNextFuncAddr(uint64_t next);

    llvm::Error translate(Function* func);

private:
    Context* _ctx;
    ConstSectionPtr _section;
    uint64_t _nextFuncAddr = 0;

    llvm::ArrayRef<uint8_t> _bytes;

    struct Func {
        std::string name;
        uint64_t start;
        uint64_t end;
    };

    llvm::Error translate(const Func& func);
};

}

#endif
