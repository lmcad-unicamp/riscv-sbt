#ifndef SBT_MODULE_H
#define SBT_MODULE_H

#include "Context.h"
#include "Function.h"
#include "Object.h"

#include <llvm/Support/Error.h>

#include <map>
#include <memory>

namespace sbt {

class SBTSection;

// this class represents a program module
// (right now, corresponds to one object file)
class Module
{
public:
    Module(Context* ctx);
    ~Module();

    // translate object file
    llvm::Error translate(const std::string& file);

    SBTSection* lookupSection(const std::string& name) const
    {
        auto it = _sectionMap.find(name);
        if (it == _sectionMap.end())
            return nullptr;
        return it->second.get();
    }

private:
    Context* _ctx;
    ConstObjectPtr _obj = nullptr;
    std::unique_ptr<ShadowImage> _shadowImage;
    std::map<std::string, std::unique_ptr<SBTSection>> _sectionMap;

    // methods

    void start();
    void finish();
};

}

#endif
