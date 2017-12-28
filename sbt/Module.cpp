#include "Module.h"

#include "Builder.h"
#include "Function.h"
#include "SBTError.h"
#include "Section.h"
#include "ShadowImage.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormatVariadic.h>

#include <algorithm>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

Module::Module(Context* ctx)
    :
    _ctx(ctx)
{}


Module::~Module()
{
}


void Module::start()
{
    _shadowImage.reset(new ShadowImage(_ctx, _obj));
    _ctx->shadowImage = _shadowImage.get();
}


llvm::Error Module::translate(const std::string& file)
{
    // parse object file
    auto expObj = create<Object>(file);
    if (!expObj)
        return expObj.takeError();
    if (auto err = expObj.get().readSymbols())
        return err;
    _obj = &expObj.get();

    start();

    // translate each section
    for (ConstSectionPtr sec : _obj->sections()) {
        SBTSection ssec(_ctx, sec);
        if (auto err = ssec.translate())
            return err;
    }

    // finish module
    finish();

    return llvm::Error::success();
}

}
