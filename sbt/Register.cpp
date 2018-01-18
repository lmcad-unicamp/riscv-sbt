#include "Register.h"

#include "Builder.h"

namespace sbt {

Register::Register(
    Context* ctx,
    unsigned num,
    const std::string& name,
    Type type,
    uint32_t flags)
    :
    _name(name),
    _local(flags & LOCAL)
{
    if (type == T_INT && num == 0) {
        _r = nullptr;
        return;
    }

    llvm::Type* lltype;
    llvm::Constant* zero;
    if (type == T_FLOAT) {
        lltype = llvm::Type::getDoubleTy(*ctx->ctx);
        zero = llvm::ConstantFP::get(lltype, 0.0);
    } else {
        lltype = llvm::Type::getInt32Ty(*ctx->ctx);
        zero = llvm::ConstantInt::get(lltype, 0);
    }

    const bool decl = flags & DECL;

    // get reg name and linkage type
    if (_local) {
        Builder* bld = ctx->bld;
        xassert(bld);
        _r = bld->_alloca(lltype, nullptr, _name);

    // global
    } else {
        llvm::GlobalVariable::LinkageTypes linkt =
            llvm::GlobalVariable::ExternalLinkage;
        _r = new llvm::GlobalVariable(*ctx->module, lltype, !CONSTANT,
            linkt, decl? nullptr : zero, name);
    }
}

}
