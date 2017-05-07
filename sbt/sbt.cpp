#include "sbt.h"

#include "Constants.h"
#include "Object.h"
#include "SBTError.h"
#include "Translator.h"
#include "Utils.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

using namespace llvm;

// RISCVMaster initializers
extern "C" void LLVMInitializeRISCVMasterAsmParser();
extern "C" void LLVMInitializeRISCVMasterDisassembler();
extern "C" void LLVMInitializeRISCVMasterTargetMC();
extern "C" void LLVMInitializeRISCVMasterTarget();
extern "C" void LLVMInitializeRISCVMasterTargetInfo();

namespace llvm {
Target& getTheRISCVMaster32Target();
}


namespace sbt {

void SBT::init()
{
  Object::init();

  // print stack trace if we signal out
  sys::PrintStackTraceOnErrorSignal("");

  // Init LLVM targets
  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();

  // Init RISCVMaster 'target'
  LLVMInitializeRISCVMasterTargetInfo();
  LLVMInitializeRISCVMasterTargetMC();
  LLVMInitializeRISCVMasterAsmParser();
  LLVMInitializeRISCVMasterDisassembler();
  // skip registration of RISCVMaster target to avoid
  // conflicts with RISCV target
  // LLVMInitializeRISCVMasterTarget();
}


void SBT::finish()
{
  llvm_shutdown();
  Object::finish();
}


SBT::SBT(
  const cl::list<std::string>& inputFilesList,
  const std::string& outputFile,
  Error& err)
  :
  _outputFile(outputFile),
  _context(new LLVMContext),
  _builder(new IRBuilder<>(*_context)),
  _module(new llvm::Module("main", *_context)),
  _translator(new Translator(&*_context, &*_builder, &*_module))
{
  _translator->setOutputFile(outputFile);
  for (const auto& file : inputFilesList) {
    if (!sys::fs::exists(file)) {
      SBTError serr(file);
      serr << "No such file.";
      err = error(serr);
      return;
    }
    _translator->addInputFile(file);
  }

  SBTError serr;
  auto error = [&err](SBTError& serr) {
    err = sbt::error(std::move(serr));
  };

  // get target
  Triple triple("riscv32-unknown-elf");
  std::string tripleName = triple.getTriple();
  // logs() << "Triple: " << TripleName << "\n";

  _target = &getTheRISCVMaster32Target();
  // Target = TargetRegistry::lookupTarget(TripleName, StrError);
  if (!_target) {
    serr << "Target not found: " << tripleName;
    error(serr);
    return;
  }

  _mri.reset(_target->createMCRegInfo(tripleName));
  if (!_mri) {
    serr << "No register info for target " << tripleName;
    error(serr);
    return;
  }

  // Set up disassembler.
  _asmInfo.reset(_target->createMCAsmInfo(*_mri, tripleName));
  if (!_asmInfo) {
    serr << "No assembly info for target " << tripleName;
    error(serr);
    return;
  }

  SubtargetFeatures features;
  _sti.reset(
      _target->createMCSubtargetInfo(tripleName, "", features.getString()));
  if (!_sti) {
    serr << "No subtarget info for target " << tripleName;
    error(serr);
    return;
  }

  _mofi.reset(new MCObjectFileInfo);
  _mc.reset(new MCContext(_asmInfo.get(), _mri.get(), _mofi.get()));
  _disAsm.reset(_target->createMCDisassembler(*_sti, *_mc));
  if (!_disAsm) {
    serr << "No disassembler for target " << tripleName;
    error(serr);
    return;
  }

  _mii.reset(_target->createMCInstrInfo());
  if (!_mii) {
    serr << "No instruction info for target " << tripleName;
    error(serr);
    return;
  }

  _instPrinter.reset(
    _target->createMCInstPrinter(triple, 0, *_asmInfo, *_mii, *_mri));
  if (!_instPrinter) {
    serr << "No instruction printer for target " << tripleName;
    error(serr);
    return;
  }
}


Error SBT::run()
{
  // setup translator
  _translator->setDisassembler(&*_disAsm);
  _translator->setInstPrinter(&*_instPrinter);
  _translator->setSTI(&*_sti);

  if (auto err = _translator->translate())
    return err;

  if (llvm::verifyModule(*_module, &DBGS)) {
    SBTError serr;
    serr << "Translation produced invalid bitcode!";
    dump();
    return error(serr);
  }

  return Error::success();
}


void SBT::dump() const
{
  _module->dump();
}


void SBT::write()
{
  std::error_code ec;
  raw_fd_ostream os(_outputFile, ec, sys::fs::F_None);
  WriteBitcodeToFile(&*_module, os);
  os.flush();
}


llvm::Error SBT::genHello()
{
  // constants
  static const int SYS_EXIT = 1;
  static const int SYS_WRITE = 4;

  // types
  Type *Int8 = Type::getInt8Ty(*_context);
  Type *Int32 = Type::getInt32Ty(*_context);

  // syscall
  FunctionType *ftSyscall =
    FunctionType::get(Int32, { Int32, Int32 }, VAR_ARG);
  Function *fSyscall = Function::Create(ftSyscall,
      Function::ExternalLinkage, "syscall", &*_module);

  // set data
  std::string hello("Hello, World!\n");
  GlobalVariable *msg =
    new GlobalVariable(
      *_module,
      ArrayType::get(Int8, hello.size()),
      CONSTANT,
      GlobalVariable::PrivateLinkage,
      ConstantDataArray::getString(*_context, hello.c_str(), !ADD_NULL),
      "msg");

  // _start
  FunctionType *ftStart = FunctionType::get(Int32, {}, !VAR_ARG);
  Function *fStart =
    Function::Create(ftStart, Function::ExternalLinkage, "_start", &*_module);

  // entry basic block
  BasicBlock *bb = BasicBlock::Create(*_context, "entry", fStart);
  _builder->SetInsertPoint(bb);

  // call write
  Value *sc = ConstantInt::get(Int32, APInt(32, SYS_WRITE, SIGNED));
  Value *fd = ConstantInt::get(Int32, APInt(32, 1, SIGNED));
  Value *ptr = msg;
  Value *len = ConstantInt::get(Int32, APInt(32, hello.size(), SIGNED));
  std::vector<Value *> args = { sc, fd, ptr, len };
  _builder->CreateCall(fSyscall, args);

  // call exit
  sc = ConstantInt::get(Int32, APInt(32, SYS_EXIT, SIGNED));
  Value *rc = ConstantInt::get(Int32, APInt(32, 0, SIGNED));
  args = { sc, rc };
  _builder->CreateCall(fSyscall, args);
  _builder->CreateRet(rc);

  _module->dump();
  std::exit(EXIT_FAILURE);

  return Error::success();
}


Error SBT::genSCHandler()
{
  if (auto err = _translator->genSCHandler())
    return err;
  write();
  return Error::success();
}


///

struct SBTFinish
{
  ~SBTFinish()
  {
    sbt::SBT::finish();
  }
};


static void handleError(Error&& err)
{
  Error err2 = llvm::handleErrors(std::move(err),
    [](const SBTError& serr) {
      serr.log(errs());
      std::exit(EXIT_FAILURE);
    });

  if (err2) {
    logAllUnhandledErrors(std::move(err2), errs(), *BIN_NAME + ": error: ");
    std::exit(EXIT_FAILURE);
  }
}

} // sbt


static void test()
{
#if 1
  using namespace sbt;

  // ExitOnError ExitOnErr;
  SBT::init();
  SBTFinish finish;
  StringRef filePath = "sbt/test/rv32-main.o";
  auto expObj = create<Object>(filePath);
  if (!expObj)
    handleError(expObj.takeError());
  else
    expObj.get().dump();
  std::exit(EXIT_FAILURE);
#endif
}


int main(int argc, char *argv[])
{
  // initialize constants
  sbt::initConstants();
  struct DestroyConstants {
    ~DestroyConstants() {
      sbt::destroyConstants();
    }
  } destroyConstants;

  // options
  cl::list<std::string> inputFiles(
      cl::Positional,
      cl::desc("<input object files>"),
      cl::ZeroOrMore);

  cl::opt<std::string> outputFileOpt(
      "o",
      cl::desc("output filename"));

  cl::opt<bool> genSCHandlerOpt(
      "gen-sc-handler",
      cl::desc("generate syscall handler"));

  cl::opt<bool> testOpt("test");

  // parse args
  cl::ParseCommandLineOptions(argc, argv);

  // gen syscall handlers
  if (genSCHandlerOpt) {
    if (outputFileOpt.empty()) {
      errs() << *sbt::BIN_NAME
        << ": output file not specified.\n";
      return 1;
    }
  // debug test
  } else if (testOpt) {
    test();
  // translate
  } else {
    if (inputFiles.empty()) {
      errs() << *sbt::BIN_NAME << ": no input files.\n";
      return 1;
    }
  }

  // set output file
  std::string outputFile;
  if (outputFileOpt.empty()) {
    outputFile = inputFiles.front();
    // remove file extension
    outputFile = outputFile.substr(0, outputFile.find_last_of('.')) + ".bc";
  } else
    outputFile = outputFileOpt;

  // start SBT
  sbt::SBT::init();
  sbt::SBTFinish fini;

  // create SBT
  auto exp = sbt::create<sbt::SBT>(inputFiles, outputFile);
  if (!exp)
    sbt::handleError(exp.takeError());
  sbt::SBT& sbt = exp.get();

  // translate files
  if (genSCHandlerOpt) {
    sbt::handleError(sbt.genSCHandler());
    return EXIT_SUCCESS;
  } else
    sbt::handleError(sbt.run());

  // dump resulting IR
  sbt.dump();

  // write IR to output file
  sbt.write();

  return EXIT_SUCCESS;
}
