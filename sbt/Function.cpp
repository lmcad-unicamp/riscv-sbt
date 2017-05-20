#include "Function.h"

#include "BasicBlock.h"
#include "Builder.h"
#include "Constants.h"
#include "SBTError.h"
#include "Section.h"
#include "Stack.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormatVariadic.h>

namespace sbt {

llvm::Error Function::create(
  llvm::FunctionType* ft,
  llvm::Function::LinkageTypes linkage)
{
  _f = _ctx->module->getFunction(_name);
  if (_f)
    return llvm::Error::success();

  // create a function with no parameters
  if (!ft)
    ft = llvm::FunctionType::get(_ctx->t.voidT, !VAR_ARG);
  _f = llvm::Function::Create(ft, linkage, _name, _ctx->module);

  return llvm::Error::success();
}


llvm::Error Function::translate()
{
  DBGS << _name << ":\n";

  if (_name == "main") {
    if (auto err = startMain())
      return err;
  } else {
    if (auto err = start())
      return err;
  }

  if (auto err = translateInstrs(_addr, _end))
    return err;

  if (auto err = finish())
    return err;

  return llvm::Error::success();
}


llvm::Error Function::startMain()
{
  const Types& t = _ctx->t;
  auto builder = _ctx->builder;
  Builder bld(_ctx);

  // create a function with no parameters
  llvm::FunctionType *ft =
    llvm::FunctionType::get(t.i32, !VAR_ARG);
  _f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
    _name, _ctx->module);

  // bb
  BasicBlock bb(_ctx, _addr, _f);
  _bbMap(_addr, std::move(bb));
  _bb = _bbMap[_addr];
  bld.setInsertPoint(*_bb);

  // set stack pointer
  bld.store(_ctx->stack->end(), XRegister::SP);

  // init syscall module
  llvm::Function* f = llvm::Function::Create(t.voidFunc,
    llvm::Function::ExternalLinkage, "syscall_init", _ctx->module);
  builder->CreateCall(f);

  return llvm::Error::success();
}


llvm::Error Function::start()
{
  if (auto err = create())
    return err;

  /*
  // bb
  _bb = _bbMap[_addr];
  if (!_bb) {
    _bbMap(_addr, BasicBlock(_ctx, _addr, _f));
    _bb = _bbMap[_addr];
  } else {
    auto b = _bb->bb();
    b->removeFromParent();
    b->insertInto(_f);
  }
  */

  _bbMap(_addr, BasicBlock(_ctx, _addr, _f));
  _bb = _bbMap[_addr];
  Builder bld(_ctx);
  bld.setInsertPoint(*_bb);

  return llvm::Error::success();
}


llvm::Error Function::finish()
{
  auto builder = _ctx->builder;
  if (builder->GetInsertBlock()->getTerminator() == nullptr)
    builder->CreateRetVoid();
  return llvm::Error::success();
}


llvm::Error Function::translateInstrs(uint64_t st, uint64_t end)
{
  DBGS << __FUNCTION__ << llvm::formatv("({0:X+4}, {1:X+4})\n", st, end);

  ConstSectionPtr section = _sec->section();
  const llvm::ArrayRef<uint8_t> bytes = _sec->bytes();

  // for each instruction
  uint64_t size = Instruction::SIZE;
  for (uint64_t addr = st; addr < end; addr += size) {
    // disasm
    const uint8_t* rawBytes = &bytes[addr];
    uint32_t rawInst = *reinterpret_cast<const uint32_t*>(rawBytes);

    // consider 0 bytes as end-of-section padding
    if (_state == ST_PADDING) {
      if (rawInst != 0) {
        SBTError serr;
        serr << "Found non-zero byte in zero-padding area";
        return error(serr);
      }
      continue;
    } else if (rawInst == 0) {
      _state = ST_PADDING;
      continue;
    }

    /* update current bb
     *
    if (NextBB && CurAddr == NextBB) {
      BasicBlock **BB = BBMap[CurAddr];
      assert(BB && "BasicBlock not found!");
      if (!BrWasLast)
        Builder->CreateBr(*BB);
      Builder->SetInsertPoint(*BB);

      auto Iter = BBMap.lower_bound(CurAddr + 4);
      if (Iter != BBMap.end())
        updateNextBB(Iter->key);
    }

    BrWasLast = false;
    */

    Instruction inst(_ctx, addr, rawInst);
    if (auto err = inst.translate())
      return err;
    (*_bb)(addr, std::move(inst));
  }

  return llvm::Error::success();
}

}
