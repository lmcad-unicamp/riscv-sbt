#include "Module.h"

#include "SBTError.h"
#include "Section.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>

namespace sbt {

llvm::Error Module::start()
{
  if (auto err = buildShadowImage())
    return err;

  /*
  auto ExpF = createFunction("rv32_icaller");
  if (!ExpF)
    return ExpF.takeError();
  ICaller = ExpF.get();

  FunctionType *FT = FunctionType::get(I32, !VAR_ARG);
  GetCycles = Function::Create(FT,
    Function::ExternalLinkage, "get_cycles", Module);

  GetTime = Function::Create(FT,
    Function::ExternalLinkage, "get_time", Module);

  InstRet = Function::Create(FT,
    Function::ExternalLinkage, "get_instret", Module);
  */

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
/*
  PointerType *Ty = VoidFun->getPointerTo();
  Value *Target = nullptr;

  // basic blocks
  BasicBlock *BBPrev = Builder->GetInsertBlock();
  BasicBlock *BBBeg = BasicBlock::Create(*Context, "begin", ICaller);
  BasicBlock *BBDfl = BasicBlock::Create(*Context, "default", ICaller);
  BasicBlock *BBEnd = BasicBlock::Create(*Context, "end", ICaller);

  // default:
  // t1 = nullptr;
  Builder->SetInsertPoint(BBDfl);
  store(ZERO, RV_T1);
  Builder->CreateBr(BBEnd);

  // end: call t1
  Builder->SetInsertPoint(BBEnd);
  Target = load(RV_T1);
  Target = Builder->CreateIntToPtr(Target, Ty);
  Builder->CreateCall(Target);
  Builder->CreateRetVoid();

  // switch
  Builder->SetInsertPoint(BBBeg);
  Target = load(RV_T1);
  SwitchInst *SW = Builder->CreateSwitch(Target, BBDfl, FunTable.size());

  // cases
  // case fun: t1 = realFunAddress;
  for (Function *F : FunTable) {
    uint64_t *Addr = FunMap[F];
    assert(Addr && "Indirect called function not found!");
    std::string CaseStr = "case_" + F->getName().str();
    Value *Sym = Module->getValueSymbolTable().lookup(F->getName());

    BasicBlock *Dest = BasicBlock::Create(*Context, CaseStr, ICaller, BBDfl);
    Builder->SetInsertPoint(Dest);
    Sym = Builder->CreatePtrToInt(Sym, I32);
    store(Sym, RV_T1);
    Builder->CreateBr(BBEnd);

    Builder->SetInsertPoint(BBBeg);
    SW->addCase(ConstantInt::get(I32, *Addr), Dest);
  }

  Builder->SetInsertPoint(BBPrev);
*/
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
