#ifndef SBT_RELOCATION_H
#define SBT_RELOCATION_H

#include "Context.h"
#include "Object.h"
#include "Symbol.h"

#include <llvm/IR/Value.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

namespace sbt {

// relocator
class SBTRelocation
{
public:
    using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

    /**
     * @param ctx
     * @param ri relocation begin (iterator)
     * @param re relocation end
     */
    SBTRelocation(
        Context* ctx,
        ConstRelocIter ri,
        ConstRelocIter re);

    /**
     * Handle relocation, by checking if there is a relocation for the
     * given address, and if so returning an llvm::Value corresponding to
     * the "relocated" value.
     *
     * @param addr address
     * @param os output stream to print debug info
     *
     * @return "relocated" value if there was a relocation to 'addr',
     *                 or null otherwise.
     */
    llvm::Expected<llvm::Value*>
    handleRelocation(uint64_t addr, llvm::raw_ostream* os);

    void skipRelocation(uint64_t addr);

    /**
     * Move to next relocation.
     *
     * @param addr current address
     * @param hadNext was there a previous relocation
     *        before the current one?
     */
    void next(uint64_t addr, bool hadNext);

    /**
     * Get last "relocated" symbol.
     */
    const SBTSymbol& last() const
    {
        return _last;
    }

    /**
     * Check if is there a symbol at 'addr'.
     */
    bool isSymbol(uint64_t addr) const
    {
        return _hasSymbol && _last.instrAddr == addr;
    }

private:
    Context* _ctx;
    ConstRelocIter _ri;
    ConstRelocIter _re;
    ConstRelocIter _rlast;
    SBTSymbol _last;
    bool _hasSymbol = false;
    uint64_t _next = Constants::INVALID_ADDR;
};

}

#endif
