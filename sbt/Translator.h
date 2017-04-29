#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Constants.h"
#include "Map.h"
#include "Object.h"
#include "Register.h"
#include "SBTError.h"

#include <llvm/MC/MCInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ELF.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FormatVariadic.h>

namespace llvm {
class Function;
class FunctionType;
class GlobalVariable;
class IntegerType;
class LLVMContext;
class MCDisassembler;
class MCInst;
class MCInstPrinter;
class MCSubtargetInfo;
class Module;
class Value;
}

namespace sbt {

class Translator
{
public:
  static const size_t InstrSize = 4;

  // ctor
  Translator(
    llvm::LLVMContext *ctx,
    llvm::IRBuilder<> *bldr,
    llvm::Module *mod);

  // move only
  Translator(Translator&&) = default;
  Translator(const Translator&) = delete;

  // gen syscall handler
  llvm::Error genSCHandler();

  // translate file
  llvm::Error translate(const std::string& file);

  // setters

  void setDisassembler(const llvm::MCDisassembler* d)
  {
    _disAsm = d;
  }

  void setInstPrinter(llvm::MCInstPrinter* p)
  {
    _instPrinter = p;
  }

  void setSTI(const llvm::MCSubtargetInfo* s)
  {
    _sti = s;
  }

private:

  // types

  enum ALUOp {
    ADD,
    AND,
    MUL,
    OR,
    SLL,
    SLT,
    SRA,
    SRL,
    SUB,
    XOR
  };

  enum ALUOpFlags {
    AF_NONE = 0,
    AF_IMM = 1,
    AF_UNSIGNED = 2
  };

  enum UIOp {
    AUIPC,
    LUI
  };

  enum IntType {
    S8,
    U8,
    S16,
    U16,
    U32
  };

  enum BranchType {
    JAL,
    JALR,
    BEQ,
    BNE,
    BGE,
    BGEU,
    BLT,
    BLTU
  };

  enum CSROp {
    RW,
    RS,
    RC
  };

  using ConstRelocIter = ConstRelocationPtrVec::const_iterator;

  // Symbol

  class Symbol
  {
  public:
    const std::string& name() const
    {
      return _name;
    }

    uint64_t addr() const
    {
      return _addr;
    }

    uint64_t val() const
    {
      return _val;
    }

    ConstSectionPtr sec() const
    {
      return _sec;
    }

    bool isExternal() const
    {
      return addr() == 0 && !sec();
    }

  private:
    std::string _name;
    uint64_t _addr = 0;
    uint64_t _val = 0;
    ConstSectionPtr _sec = nullptr;
  };

  // Immediate

  struct Imm
  {
    bool isSym;
    Symbol sym;
  };

  // State

  enum TranslationState {
    ST_DFL,
    ST_PADDING
  };

  // per program state
  bool _inMain = false;

  struct Module
  {
    llvm::Error startModule();
    llvm::Error finishModule();

    TranslationState _state = ST_DFL;
    ConstObjectPtr _obj = nullptr;
    Map<llvm::Function *, uint64_t> _funMap;
    std::vector<llvm::Function *> _funTable;
    // icaller
    llvm::Function *ICaller = nullptr;
    uint64_t ExtFunAddr = 0;
  };

  struct Section
  {
    void setRelocIters(ConstRelocIter RI, ConstRelocIter RE)
    {
      _ri = RI;
      _re = RE;
      _rlast = RI;
    }

    ConstSectionPtr _section;
    llvm::ArrayRef<uint8_t> _sectionBytes;
    llvm::Function* _func = nullptr;
    ConstRelocIter _ri;
    ConstRelocIter _re;
    ConstRelocIter _rlast;
  };

  class Function
  {
    llvm::Expected<llvm::Function *> createFunction(llvm::StringRef Name);
    llvm::Error startMain(llvm::StringRef Name, uint64_t Addr);
    llvm::Error startFunction(llvm::StringRef Name, uint64_t Addr);
    llvm::Error finishFunction();
    llvm::Expected<uint64_t> import(llvm::StringRef func);

    uint64_t _addr = 0;
    Map<uint64_t, llvm::BasicBlock *> _bbMap;
    Map<uint64_t, llvm::Instruction *> _instrMap;
    uint64_t _nextBB = 0;
    bool _brWasLast = false;
  };

  class SBTBasicBlock
  {
  public:
    static std::string getBBName(uint64_t Addr)
    {
      std::string BBName = "bb";
      llvm::raw_string_ostream SS(BBName);
      SS << llvm::Twine::utohexstr(Addr);
      SS.flush();
      return BBName;
    }

    static llvm::BasicBlock* create(
      llvm::LLVMContext& context,
      uint64_t addr,
      llvm::Function* f,
      llvm::BasicBlock* beforeBB = nullptr)
    {
      return create(context, getBBName(addr), f, beforeBB);
    }

    static llvm::BasicBlock* create(
      llvm::LLVMContext& context,
      llvm::StringRef name,
      llvm::Function* f,
      llvm::BasicBlock* beforeBB = nullptr)
    {
      DBGS << name << ":\n";
      return llvm::BasicBlock::Create(context, name, f, beforeBB);
    }

    llvm::BasicBlock *splitBB(
      llvm::BasicBlock *BB,
      uint64_t Addr);

    void updateNextBB(uint64_t Addr)
    {
      if (NextBB <= CurAddr || Addr < NextBB)
        NextBB = Addr;
    }
  };

  struct Instruction
  {
    // get RD
    unsigned getRD(const llvm::MCInst &Inst, llvm::raw_ostream &SS)
    {
      return getRegNum(0, Inst, SS);
    }

    // GetRegNum
    unsigned getRegNum(
      unsigned Num,
      const llvm::MCInst &Inst,
      llvm::raw_ostream &SS)
    {
      const llvm::MCOperand &R = Inst.getOperand(Num);
      unsigned NR = RVReg(R.getReg());
      SS << regName(NR) << ", ";
      return NR;
    }

    // Get register
    llvm::Value *getReg(
      const llvm::MCInst &Inst,
      int Op,
      llvm::raw_ostream &SS)
    {
      const llvm::MCOperand &OR = Inst.getOperand(Op);
      unsigned NR = RVReg(OR.getReg());
      llvm::Value *V;
      if (NR == 0)
        V = ZERO;
      else
        V = load(NR);

      SS << regName(NR);
      if (Op < 2)
         SS << ", ";
      return V;
    }

    // Get register or immediate
    llvm::Expected<llvm::Value *> getRegOrImm(
      const llvm::MCInst &Inst,
      int Op,
      llvm::raw_ostream &SS)
    {
      const llvm::MCOperand &O = Inst.getOperand(Op);
      if (O.isReg())
        return getReg(Inst, Op, SS);
      else if (O.isImm())
        return getImm(Inst, Op, SS);
      else
        llvm_unreachable("Operand is neither a Reg nor Imm");
    }

    // Get immediate
    llvm::Expected<llvm::Value *> getImm(
      const llvm::MCInst &Inst,
      int Op,
      llvm::raw_ostream &SS)
    {
      auto ExpV = handleRelocation(SS);
      if (!ExpV)
        return ExpV.takeError();
      llvm::Value *V = ExpV.get();
      if (V)
        return V;

      int64_t Imm = Inst.getOperand(Op).getImm();
      V = llvm::ConstantInt::get(I32, Imm);
      SS << llvm::formatv("0x{0:X-4}", uint32_t(Imm));
      return V;
    }


    // per instruction state
    llvm::Instruction* _first = nullptr;
    Imm _lastImm;
  };

  // Builder

  struct Builder
  {
    void updateFirst(llvm::Value *V)
    {
      if (!First)
        First = llvm::dyn_cast<llvm::Instruction>(V);
    }

    // Load register
    llvm::LoadInst *load(unsigned Reg)
    {
      llvm::LoadInst *I = Builder->CreateLoad(X[Reg], X[Reg]->getName() + "_");
      updateFirst(I);
      return I;
    }

    // Store value to register
    llvm::StoreInst *store(llvm::Value *V, unsigned Reg)
    {
      if (Reg == 0)
        return nullptr;

      llvm::StoreInst *I = Builder->CreateStore(V, X[Reg], !VOLATILE);
      updateFirst(I);
      return I;
    }

    // Add
    llvm::Value *add(llvm::Value *O1, llvm::Value *O2)
    {
      llvm::Value *V = Builder->CreateAdd(O1, O2);
      updateFirst(V);
      return V;
    }

    // nop
    void nop()
    {
      load(RV_ZERO);
    }

    llvm::Value *i8PtrToI32(llvm::Value *V8)
    {
      // Cast to int32_t *
      llvm::Value *V =
        Builder->CreateCast(llvm::Instruction::CastOps::BitCast,
          V8, llvm::Type::getInt32PtrTy(*Context));
      updateFirst(V);
      // Cast to int32_t
      V = Builder->CreateCast(llvm::Instruction::CastOps::PtrToInt,
        V, I32);
      updateFirst(V);
      return V;
    }
  };

  // data

  // set by ctor
  llvm::LLVMContext *_ctx;
  llvm::IRBuilder<> *_builder;
  llvm::Module *_module;

  // set by sbt
  const llvm::MCDisassembler *_disAsm = nullptr;
  llvm::MCInstPrinter *_instPrinter = nullptr;
  const llvm::MCSubtargetInfo *_sti = nullptr;

  // internal

  // libC module
  std::unique_ptr<llvm::Module> _lcModule;

  // image stuff
  llvm::GlobalVariable* _shadowImage = nullptr;
  llvm::GlobalVariable* _stack = nullptr;
  llvm::Value* _stackEnd = nullptr;
  size_t _stackSize = 4096;
  ConstSectionPtr _bss = nullptr;
  size_t _bssSize = 0;

  // riscv syscall function
  llvm::FunctionType* ftRVSC;
  llvm::Function* fRVSC;

  // host dependent ops
  llvm::Function* _getCycles = nullptr;
  llvm::Function* _getTime = nullptr;
  llvm::Function* _instRet = nullptr;

  //
  // Methods
  //

  llvm::Error startup();


  bool inFunc() const
  {
    return _func;
  }

  // register file

  llvm::Error declOrBuildRegisterFile(bool decl);

  llvm::Error declRegisterFile()
  {
    return declOrBuildRegisterFile(true);
  }

  llvm::Error buildRegisterFile()
  {
    return declOrBuildRegisterFile(false);
  }

  // image
  llvm::Error buildShadowImage();
  llvm::Error buildStack();

  // syscall handler
  void declSyscallHandler();
  llvm::Error genSyscallHandler();
  llvm::Error handleSyscall();

  // indirect function caller
  llvm::Error genICaller();

  llvm::Expected<llvm::Value*> handleRelocation(llvm::raw_ostream &SS);

  // translation

  llvm::Error translateSection(ConstSectionPtr sec);
  llvm::Error translateSymbol(size_t si);
  llvm::Error translateInstrs(uint64_t begin, uint64_t end);
  llvm::Error translateInstr(const llvm::MCInst& inst);

  // ALU op

  llvm::Error translateALUOp(
    const llvm::MCInst &Inst,
    ALUOp Op,
    uint32_t Flags,
    llvm::raw_string_ostream &SS);

  llvm::Error translateUI(
    const llvm::MCInst &Inst,
    UIOp UOP,
    llvm::raw_string_ostream &SS);

  // load/store

  llvm::Error translateLoad(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

  llvm::Error translateStore(
    const llvm::MCInst &Inst,
    IntType IT,
    llvm::raw_string_ostream &SS);

  // branch/jump/call handlers

  llvm::Error translateBranch(
    const llvm::MCInst &Inst,
    BranchType BT,
    llvm::raw_string_ostream &SS);

  llvm::Error handleJumpToOffs(
    uint64_t Target,
    llvm::Value *Cond,
    unsigned LinkReg);

  llvm::Error handleIJump(
    llvm::Value *Target,
    unsigned LinkReg);

  llvm::Error handleCall(uint64_t Target);
  llvm::Error handleICall(llvm::Value *Target);
  llvm::Error handleCallExt(llvm::Value *Target);

  // fence
  llvm::Error translateFence(const llvm::MCInst &Inst,
      bool FI,
      llvm::raw_string_ostream &SS);

  // CSR ops

  llvm::Error translateCSR(
      const llvm::MCInst &Inst,
      CSROp Op,
      bool Imm,
      llvm::raw_string_ostream &SS);

#if SBT_DEBUG
  // Add RV Inst metadata and print it in debug mode
  void dbgprint(llvm::raw_string_ostream &SS);
#else
  void dbgprint(const llvm::raw_string_ostream &SS) {}
#endif
};

} // sbt

#endif
