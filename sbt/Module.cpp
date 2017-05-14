#include "Module.h"

#include "Builder.h"
#include "Function.h"
#include "SBTError.h"
#include "Section.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

namespace sbt {

Module::Module(Context* ctx)
  :
  _ctx(ctx),
  _iCaller(_ctx, "rv32_icaller")
{}


llvm::Error Module::start()
{
  if (auto err = buildShadowImage())
    return err;

  if (auto err = _iCaller.create())
    return err;

  return llvm::Error::success();
}


llvm::Error Module::finish()
{
  return genICaller();
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


llvm::Error Module::genICaller()
{
  llvm::LLVMContext& ctx = *_ctx->ctx;
  const Constants& c = _ctx->c;
  const Types& t = _ctx->t;
  auto builder = _ctx->builder;
  Builder bld(_ctx);
  llvm::Function* ic = _iCaller.func();

  llvm::PointerType* ty = t.voidFunc->getPointerTo();
  llvm::Value* target = nullptr;

  // basic blocks
  llvm::BasicBlock* bbPrev = builder->GetInsertBlock();
  llvm::BasicBlock* bbBeg = llvm::BasicBlock::Create(ctx, "begin", ic);
  llvm::BasicBlock* bbDfl = llvm::BasicBlock::Create(ctx, "default", ic);
  llvm::BasicBlock* bbEnd = llvm::BasicBlock::Create(ctx, "end", ic);

  // default:
  // t1 = nullptr;
  builder->SetInsertPoint(bbDfl);
  bld.store(c.ZERO, XRegister::T1);
  builder->CreateBr(bbEnd);

  // end: call t1
  builder->SetInsertPoint(bbEnd);
  target = bld.load(XRegister::T1);
  target = builder->CreateIntToPtr(target, ty);
  builder->CreateCall(target);
  bld.retVoid();

  // switch
  builder->SetInsertPoint(bbBeg);
  target = bld.load(XRegister::T1);
  llvm::SwitchInst* sw = builder->CreateSwitch(target, bbDfl, _funMap.size());

  // cases
  // case fun: t1 = realFunAddress;
  for (const auto& p : _funMap) {
    const FunctionPtr& f = p.key;
    uint64_t addr = p.val;

    std::string caseStr = "case_" + f->name();
    llvm::Value* sym = _ctx->module->getValueSymbolTable().lookup(f->name());

    llvm::BasicBlock* dest =
      llvm::BasicBlock::Create(ctx, caseStr, ic, bbDfl);
    builder->SetInsertPoint(dest);
    sym = builder->CreatePtrToInt(sym, t.i32);
    bld.store(sym, XRegister::T1);
    builder->CreateBr(bbEnd);

    builder->SetInsertPoint(bbBeg);
    sw->addCase(llvm::ConstantInt::get(t.i32, addr), dest);
  }

  builder->SetInsertPoint(bbPrev);
  return llvm::Error::success();
}


llvm::Error Module::buildShadowImage()
{
  SBTError serr;

  std::vector<uint8_t> vec;
  for (ConstSectionPtr sec : _obj->sections()) {
    // Skip non text/data sections
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
      // Read contents
      if (sec->contents(bytes)) {
        serr  << __FUNCTION__ << ": failed to get section ["
              << sec->name() << "] contents";
        return error(serr);
      }
    }

    // Align all sections
    while (vec.size() % 4 != 0)
      vec.push_back(0);

    // Set Shadow Offset of Section
    sec->shadowOffs(vec.size());

    // Append to vector
    for (size_t i = 0; i < bytes.size(); i++)
      vec.push_back(bytes[i]);
  }

  // Create the ShadowImage
  llvm::Constant* cda = llvm::ConstantDataArray::get(*_ctx->ctx, vec);
  _shadowImage =
    new llvm::GlobalVariable(*_ctx->module, cda->getType(), !CONSTANT,
      llvm::GlobalValue::ExternalLinkage, cda, "ShadowMemory");

  return llvm::Error::success();
}

}
