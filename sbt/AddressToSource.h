#ifndef SBT_ADDRESS_TO_SOURCE_H
#define SBT_ADDRESS_TO_SOURCE_H

#include <llvm/Support/Error.h>

#include <string>
#include <unordered_map>

namespace sbt {

class AddressToSource
{
public:
    AddressToSource(const std::string& path, llvm::Error& err);

    const std::string& lookup(uint64_t addr) const
    {
        auto it = _a2s.find(addr);
        if (it == _a2s.end())
            return _empty;
        return it->second;
    }

private:
    std::unordered_map<uint64_t, std::string> _a2s;
    std::string _empty;
};

}

#endif
