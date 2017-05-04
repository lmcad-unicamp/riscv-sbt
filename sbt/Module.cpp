#include "Module.h"

namespace sbt {

llvm::Error Module::start()
{
  /*
  if (auto E = buildShadowImage())
    return E;

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


llvm::Error Module::translate(ConstObjectPtr obj)
{
  _obj = obj;

  if (auto err = start())
    return err;

  /*
  // translate each section
  for (ConstSectionPtr sec : _obj->sections()) {
    if (auto err = translateSection(sec))
      return err;

    // finish any pending function
    if (Error err = finishFunction())
      return err;
  }
  */

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
  /*
  SBTError SE;

  std::vector<uint8_t> Vec;
  for (ConstSectionPtr Sec : CurObj->sections()) {
    // Skip non text/data sections
    if (!Sec->isText() && !Sec->isData() && !Sec->isBSS() && !Sec->isCommon())
      continue;

    StringRef Bytes;
    std::string Z;
    // .bss/.common
    if (Sec->isBSS() || Sec->isCommon()) {
      Z = std::string(Sec->size(), 0);
      Bytes = Z;
    // others
    } else {
      // Read contents
      if (Sec->contents(Bytes)) {
        SE  << __FUNCTION__ << ": failed to get section ["
            << Sec->name() << "] contents";
        return error(SE);
      }
    }

    // Align all sections
    while (Vec.size() % 4 != 0)
      Vec.push_back(0);

    // Set Shadow Offset of Section
    Sec->shadowOffs(Vec.size());

    // Append to vector
    for (size_t I = 0; I < Bytes.size(); I++)
      Vec.push_back(Bytes[I]);
  }

  // Create the ShadowImage
  Constant *CDA = ConstantDataArray::get(*Context, Vec);
  ShadowImage =
    new GlobalVariable(*Module, CDA->getType(), !CONSTANT,
      GlobalValue::ExternalLinkage, CDA, "ShadowMemory");
  */

  return llvm::Error::success();
}

}
