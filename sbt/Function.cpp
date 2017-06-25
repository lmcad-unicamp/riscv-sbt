#include "Function.h"

#include "Builder.h"
#include "Constants.h"
#include "Instruction.h"
#include "SBTError.h"
#include "Section.h"
#include "Stack.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormatVariadic.h>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

namespace sbt {

void Function::create(
  llvm::FunctionType* ft,
  llvm::Function::LinkageTypes linkage)
{
  // if function already exists in LLVM module, do nothing
  _f = _ctx->module->getFunction(_name);
  if (_f)
    return;

  // create a function with no parameters if no function type was specified
  if (!ft)
    ft = _ctx->t.voidFunc;
  _f = llvm::Function::Create(ft, linkage, _name, _ctx->module);
}


llvm::Error Function::translate()
{
  DBGS << _name << ":\n";

  xassert(_sec && "null section pointer!");
  xassert(_addr != Constants::INVALID_ADDR);
  xassert(_end != Constants::INVALID_ADDR);

  // start
  if (_name == "main") {
    if (auto err = startMain())
      return err;
  } else {
    if (auto err = start())
      return err;
  }

  // translate instructions
  if (auto err = translateInstrs(_addr, _end))
    return err;

  // finish
  if (auto err = finish())
    return err;

  return llvm::Error::success();
}


llvm::Error Function::startMain()
{
  const Types& t = _ctx->t;
  Builder* bld = _ctx->bld;

  // int main();
  llvm::FunctionType* ft = llvm::FunctionType::get(t.i32, !VAR_ARG);
  _f = llvm::Function::Create(ft,
    llvm::Function::ExternalLinkage, _name, _ctx->module);

  // first bb
  BasicBlockPtr bb(new BasicBlock(_ctx, _addr, _f));
  _bbMap(_addr, std::move(bb));
  BasicBlock* bbptr = &**_bbMap[_addr];
  bld->setInsertPoint(bbptr);

  // set stack pointer
  bld->store(_ctx->stack->end(), XRegister::SP);

  // init syscall module
  llvm::Function* f = llvm::Function::Create(t.voidFunc,
    llvm::Function::ExternalLinkage, "syscall_init", _ctx->module);
  bld->call(f);

  _ctx->inMain = true;
  return llvm::Error::success();
}


llvm::Error Function::start()
{
  create();

  // create first bb
  _bbMap(_addr, BasicBlockPtr(new BasicBlock(_ctx, _addr, _f)));
  BasicBlock* bbptr = &**_bbMap[_addr];
  _ctx->bld->setInsertPoint(bbptr);

  return llvm::Error::success();
}


llvm::Error Function::finish()
{
  auto bld = _ctx->bld;
  // add a return if there is no terminator in current block
  if (bld->getInsertBlock()->bb()->getTerminator() == nullptr)
    bld->retVoid();
  _ctx->inMain = false;
  return llvm::Error::success();
}


llvm::Error Function::translateInstrs(uint64_t st, uint64_t end)
{
  DBGS << __FUNCTION__ << llvm::formatv("({0:X+4}, {1:X+4})\n", st, end);

  ConstSectionPtr section = _sec->section();
  const llvm::ArrayRef<uint8_t> bytes = _sec->bytes();

  // for each instruction
  // XXX assuming fixed size instructions
  uint64_t size = Instruction::SIZE;
  for (uint64_t addr = st; addr < end; addr += size) {
    // disasm
    const uint8_t* rawBytes = &bytes[addr];
    uint32_t rawInst = *reinterpret_cast<const uint32_t*>(rawBytes);

    // consider 0 bytes as end-of-section padding
    if (_state == ST_PADDING) {
      // when in padding state, all remaining function bytes should be 0
      if (rawInst != 0) {
        SBTError serr;
        serr << "found non-zero byte in zero-padding area";
        return error(serr);
      }
      continue;
    } else if (rawInst == 0) {
      _state = ST_PADDING;
      continue;
    }

    // update current bb
    DBGF("addr={0:X+8}, _nextBB={1:X+8}", addr, _nextBB);
    if (_nextBB && addr == _nextBB) {
      BasicBlock* bbptr = &**_bbMap[addr];
      xassert(bbptr && "BasicBlock not found!");
      DBGF("insertPoint={0}:", bbptr->name());

      // if last instruction was not a branch, then branch
      // from previous BB to current one
      if (!_ctx->brWasLast)
        _ctx->bld->br(bbptr);
      _ctx->bld->setInsertPoint(*bbptr);

      // set next BB pointer
      auto it = _bbMap.lower_bound(addr + size);
      if (it != _bbMap.end())
        updateNextBB(it->key);
    }
    // reset last instruction was branch flag
    _ctx->brWasLast = false;

    // translate instruction
    Instruction inst(_ctx, addr, rawInst);
    // note: Instruction::translate() may change _bbMap,
    // possibly invalidating bbptr
    if (auto err = inst.translate())
      return err;
    // DBGF("addr={0:X+8}", addr);
    // add translated instruction to BB's instruction map
    (*curBB())(addr, std::move(_ctx->bld->first()));
  }

  return llvm::Error::success();
}


Function* Function::getByAddr(Context* ctx, uint64_t addr)
{
  if (auto fp = ctx->funcByAddr(addr, !ASSERT_NOT_NULL))
    return fp;

  // get symbol by offset
  // FIXME need to change this to be able to find functions in other modules
  ConstSymbolPtr sym = ctx->sec->section()->lookup(addr);
  // XXX lookup by symbol name
  xassert(sym && "symbol not found!");

  // create a new function
  std::string name = sym->name();
  FunctionPtr f(new Function(ctx, name, ctx->sec, addr));
  f->create();
  // insert in maps
  ctx->addFunc(std::move(f));
  return ctx->func(name);
}


BasicBlock* Function::curBB()
{
  uint64_t addr = _ctx->bld->getInsertBlock()->addr();
  BasicBlock* bb = &**_bbMap[addr];
  xassert(bb && "couldn't find current basic block!");
  return bb;
}

}
