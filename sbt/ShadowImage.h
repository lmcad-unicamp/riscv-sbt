#ifndef SBT_SHADOWIMAGE_H
#define SBT_SHADOWIMAGE_H

#include "Context.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace llvm {
class Constant;
}

namespace sbt {

class BasicBlock;
class Object;

struct PendingReloc {
    struct Section {
        std::string name;
        uint64_t offs;
    };

    struct Symbol {
        uint64_t addr;
        std::string name;
    };

    Symbol sym;
    std::vector<Section> secs;

    PendingReloc(
        uint64_t symAddr,
        const std::string& symName,
        const std::string& secName,
        uint64_t secOffs)
        :
        sym{symAddr, symName},
        secs({ Section{secName, secOffs} })
    {}
};
using PendingRelocsMap = std::unordered_map<uint64_t, PendingReloc>;
using PendingRelocsIter = PendingRelocsMap::iterator;


class ShadowImage
{
public:
    ShadowImage(Context* ctx, const Object* obj);

    llvm::Constant* getSection(const std::string& name) const {
        auto it = _sections.find(name);
        xassert(it != _sections.end() && "Section not found in ShadowImage!");
        return it->second;
    }

    BasicBlock* processPending(uint64_t addr, BasicBlock* bb);
    void addPending(PendingReloc&& prel);

    bool noPendingRelocs() const {
        return _pendingRelocs.empty();
    }

private:
    Context* _ctx;
    const Object* _obj;
    std::map<std::string, llvm::Constant*> _sections;
    PendingRelocsMap _pendingRelocs;

    void build();
};

}

#endif
