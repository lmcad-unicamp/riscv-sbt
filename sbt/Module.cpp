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
    _ctx->sbtmodule = this;
    _shadowImage.reset(new ShadowImage(_ctx, _obj));
    _ctx->shadowImage = _shadowImage.get();
}


void Module::finish()
{
    _ctx->sbtmodule = nullptr;
}


llvm::Error Module::translate(const std::string& file)
{
    // parse object file
    auto expObj = create<Object>(file, _ctx->opts);
    if (!expObj)
        return expObj.takeError();
    if (auto err = expObj.get().readSymbols())
        return err;
    _obj = &expObj.get();

    for (ConstSectionPtr sec : _obj->sections())
       _sectionMap[sec->name()] =
           std::unique_ptr<SBTSection>(new SBTSection(_ctx, sec));

    start();

    // translate each section
    for (auto& p : _sectionMap) {
        SBTSection* ssec = p.second.get();
        if (auto err = ssec->translate())
            return err;
    }

    finish();
    return llvm::Error::success();
}

}
