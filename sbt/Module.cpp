#include "Module.h"

#include "Builder.h"
#include "Function.h"
#include "SBTError.h"
#include "Section.h"

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


llvm::Error Module::start()
{
  if (auto err = buildShadowImage())
    return err;

  return llvm::Error::success();
}


llvm::Error Module::finish()
{
  return llvm::Error::success();
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

  if (auto err = start())
    return err;

  // translate each section
  for (ConstSectionPtr sec : _obj->sections()) {
    SBTSection ssec(_ctx, sec);
    if (auto err = ssec.translate())
      return err;
  }

  // finish module
  if (auto err = finish())
    return err;

  return llvm::Error::success();
}


llvm::Error Module::buildShadowImage()
{
  SBTError serr;

  std::vector<uint8_t> vec;
  for (ConstSectionPtr sec : _obj->sections()) {
    // skip non text/data sections
    if (!sec->isText() && !sec->isData() && !sec->isBSS() && !sec->isCommon())
      continue;

    llvm::StringRef bytes;
    std::string z;
    // .bss/.common
    if (sec->isBSS() || sec->isCommon()) {
      z = std::string(sec->size(), 0);
      bytes = z;
    // others
    } else {
      // read contents
      if (sec->contents(bytes)) {
        serr  << __FUNCTION__ << ": failed to get section ["
              << sec->name() << "] contents";
        return error(serr);
      }
    }

    // align
    while (vec.size() % 4 != 0)
      vec.push_back(0);
    size_t addr = vec.size();
    DBGS << llvm::formatv("{0}(): {1}@{2:X+4}\n",
      __FUNCTION__, sec->name(), addr);

    // set shadow offset of section
    sec->shadowOffs(addr);

    // append to vector
    vec.resize(addr + bytes.size());
    std::copy(bytes.begin(), bytes.end(), vec.begin() + addr);
  }

  // create the ShadowImage
  llvm::Constant* cda = llvm::ConstantDataArray::get(*_ctx->ctx, vec);
  _shadowImage =
    new llvm::GlobalVariable(*_ctx->module, cda->getType(), !CONSTANT,
      llvm::GlobalValue::ExternalLinkage, cda, "ShadowMemory");
  _ctx->shadowImage = _shadowImage;

  return llvm::Error::success();
}

}
