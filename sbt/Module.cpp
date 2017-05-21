#include "Module.h"

#include "Builder.h"
#include "Function.h"
#include "SBTError.h"
#include "Section.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

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
  _obj = &expObj.get();

  if (auto err = start())
    return err;

  // translate each section
  for (ConstSectionPtr sec : _obj->sections()) {
    SBTSection ssec(sec, _ctx);
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

    // align all sections
    while (vec.size() % 4 != 0)
      vec.push_back(0);

    // set Shadow Offset of Section
    sec->shadowOffs(vec.size());

    // append to vector
    for (size_t i = 0; i < bytes.size(); i++)
      vec.push_back(bytes[i]);
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
