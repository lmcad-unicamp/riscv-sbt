#include "Function.h"

#include "Constants.h"

#include <llvm/IR/Function.h>

namespace sbt {

llvm::Expected<llvm::Function *>
Function::create(llvm::StringRef name)
{
  llvm::Function *f = module->getFunction(name);
  if (f)
    return f;

  // Create a function with no parameters
  llvm::FunctionType *ft =
    llvm::FunctionType::get(Void, !VAR_ARG);
  f = llvm::Function::Create(ft,
      llvm::Function::ExternalLinkage, name, module);

  return F;
}


llvm::Error Function::start(llvm::StringRef name, uint64_t addr)
{
  if (auto E = finish())
    return E;

  auto ExpF = create(name);
  if (!ExpF)
    return ExpF.takeError();
  llvm::Function *f = ExpF.get();
  CurFunc = F;
  FunTable.push_back(F);
  FunMap(F, std::move(Addr));

  // BB
  BasicBlock **BBPtr = BBMap[Addr];
  BasicBlock *BB;
  if (!BBPtr) {
    BB = SBTBasicBlock::create(*Context, Addr, F);
    BBMap(Addr, std::move(BB));
  } else {
    BB = *BBPtr;
    BB->removeFromParent();
    BB->insertInto(F);
  }
  Builder->SetInsertPoint(BB);

  return Error::success();
}

llvm::Error Function::finish()
{
  if (!CurFunc)
    return Error::success();
  CurFunc = nullptr;

  if (Builder->GetInsertBlock()->getTerminator() == nullptr)
    Builder->CreateRetVoid();
  InMain = false;
  return Error::success();
}

}
