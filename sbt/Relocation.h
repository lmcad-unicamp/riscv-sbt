#ifndef SBT_RELOCATION_H
#define SBT_RELOCATION_H

#include "Context.h"
#include "Object.h"
#include "Symbol.h"

#include <llvm/IR/Value.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <queue>

namespace llvm {
class GlobalVariable;
}


namespace sbt {

// relocator
class SBTRelocation
{
public:
    using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

    /**
     * ctor
     *
     * @param ctx
     * @param ri relocation begin (iterator)
     * @param re relocation end
     * @param section
     */
    SBTRelocation(
        Context* ctx,
        ConstRelocIter ri,
        ConstRelocIter re,
        ConstSectionPtr section = nullptr);

    /**
     * Handle relocation, by checking if there is a relocation for the
     * given address, and if so returning an llvm::Constant corresponding to
     * the "relocated" value.
     *
     * @param addr address
     * @param os output stream to print debug info
     *
     * @return "relocated" value if there was a relocation to 'addr',
     *         or null otherwise.
     */
    llvm::Expected<llvm::Constant*>
    handleRelocation(uint64_t addr, llvm::raw_ostream* os);

    /**
     * Skip any relocations for the given address.
     *
     * It may happen, for instance, because of ALIGN relocations
     * that may map to invalid instruction encoding bytes and we
     * just want to ignore these.
     */
    void skipRelocation(uint64_t addr);

    /**
     * Relocate the whole section at once.
     * This is used for data sections that need relocation.
     *
     * @param bytes raw section bytes
     * @param shadowImage pointer to ShadowImage
     *        (note that we can't get it from _ctx because it
     *         may not be available there yet, because this
     *         method is called when ShadowImage is being
     *         built)
     *
     * @return global variable whose content is the relocated section.
     */
    llvm::GlobalVariable* relocateSection(
        const std::vector<uint8_t>& bytes,
        const ShadowImage* shadowImage);

    bool isCall(uint64_t addr) const;
    llvm::Constant* lastSymVal() const;

private:
    Context* _ctx;
    ConstRelocIter _ri;
    ConstRelocIter _re;
    ConstSectionPtr _section;
    std::queue<ConstRelocationPtr> _proxyRelocs;
    mutable ConstRelocationPtr _cur;
    mutable ConstRelocationPtr _curP;
    ConstRelocationPtr _last = nullptr;
    llvm::Constant* _lastSymVal = nullptr;

    ConstRelocationPtr nextReloc(bool init = false);
    ConstRelocationPtr nextPReloc();
    ConstRelocationPtr getReloc(uint64_t addr);
    void addProxyReloc(ConstRelocationPtr rel, Relocation::RType rtype);
};

}

#endif
