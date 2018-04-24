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

private:
    std::unordered_map<uint64_t, std::string> _a2s;
};

}

#endif
