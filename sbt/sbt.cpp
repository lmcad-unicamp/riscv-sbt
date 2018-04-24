#include "sbt.h"

#include "Constants.h"
#include "Debug.h"
#include "Object.h"
#include "SBTError.h"
#include "Translator.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>


namespace sbt {

bool g_debug = false;

void SBT::init()
{
    Object::init();

    // print stack trace if we signal out
    llvm::sys::PrintStackTraceOnErrorSignal("");

    // init LLVM targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllDisassemblers();
}


void SBT::finish()
{
    llvm::llvm_shutdown();
    Object::finish();
}


SBT::SBT(
    const llvm::cl::list<std::string>& inputFilesList,
    const std::string& outputFile,
    const Options& opts,
    llvm::Error& err)
    :
    _outputFile(outputFile),
    _context(new llvm::LLVMContext),
    _module(new llvm::Module("main", *_context)),
    _builder(new llvm::IRBuilder<>(*_context)),
    _ctx(new Context(&*_context, &*_module, &*_builder)),
    _translator(new Translator(&*_ctx))
{
    _translator->setOutputFile(outputFile);
    for (const auto& file : inputFilesList) {
        if (!llvm::sys::fs::exists(file)) {
            err = ERRORF("file not found: {0}", file);
            return;
        }
        _translator->addInputFile(file);
    }
    _translator->setOpts(opts);
}


llvm::Error SBT::run()
{
    if (auto err = _translator->translate())
        return err;

    // check if generated bitcode is valid
    // (this outputs very helpful messages about the invalid bitcode parts)
    if (llvm::verifyModule(*_module, &DBGS)) {
        _module->dump();
        return ERROR2(InvalidBitcode, "translation produced invalid bitcode!");
    }

    return llvm::Error::success();
}


void SBT::dump() const
{
    // XXX Module::dump() is not available on LLVM release mode
    // _module->dump();
}


void SBT::write()
{
    std::error_code ec;
    llvm::raw_fd_ostream os(_outputFile, ec, llvm::sys::fs::F_None);
    WriteBitcodeToFile(&*_module, os);
    os.flush();
}


// scoped SBT 'finisher'
struct SBTFinish
{
    ~SBTFinish()
    {
        sbt::SBT::finish();
    }
};


// top level error handling function
static bool handleError(llvm::Error&& err)
{
    bool gotErrors = false;

    // handle SBTErrors
    llvm::Error err2 = llvm::handleErrors(std::move(err),
        [&gotErrors](const InvalidBitcode& serr) mutable {
            serr.log(llvm::errs());
            gotErrors = true;
            // do not exit, let the invalid bitcode be written to make
            // debugging it easier
        },
        [](const SBTError& serr) {
            serr.log(llvm::errs());
            std::exit(EXIT_FAILURE);
        });

    // handle remaining errors
    if (err2) {
        logAllUnhandledErrors(std::move(err2), llvm::errs(),
            Constants::global().BIN_NAME + ": error: ");
        std::exit(EXIT_FAILURE);
    }

    return gotErrors;
}

} // sbt


// debug/test function
static void test()
{
#if 1
    using namespace sbt;

    // ExitOnError ExitOnErr;
    SBT::init();
    SBTFinish fini;
    llvm::StringRef filePath = "sbt/test/rv32-hello.o";
    auto expObj = create<Object>(filePath);
    if (!expObj)
        handleError(expObj.takeError());
    else
        expObj.get().dump();
    std::exit(EXIT_FAILURE);
#endif
}


// main
int main(int argc, char* argv[])
{
    // options
    namespace cl = llvm::cl;

    cl::ResetCommandLineParser();

    cl::list<std::string> inputFiles(
            cl::Positional,
            cl::desc("<input object files>"),
            cl::ZeroOrMore);

    cl::opt<std::string> outputFileOpt(
            "o",
            cl::desc("Output filename"));

    cl::opt<std::string> regsOpt(
        "regs",
        cl::desc("Register translation mode: globals|locals (default=globals)"),
        cl::init("globals"));

    cl::opt<bool> dontUseLibCOpt(
        "dont-use-libc",
        cl::desc("Disallow the SBT to use libC for extended features"));

    cl::opt<bool> dontSyncFRegsOpt(
        "dont-sync-fregs",
        cl::desc("Disable F registers synchronization "
            "(useful to analyze integer only programs)"));

    cl::opt<std::string> stackSizeOpt(
        "stack-size",
        cl::desc("Stack size"),
        cl::init("4096"));

    cl::opt<std::string> a2sOpt("a2s",
        cl::desc("Address to source file to be used to insert source "
            "C code on generated assembly code"));

    // enable debug code
    cl::opt<bool> debugOpt("debug", cl::desc("Enable debug code"));

    cl::opt<bool> testOpt("test", cl::desc("For testing purposes only"));

    cl::opt<bool> helpOpt("help",
        cl::desc("Print this help message"));

    // parse args
    cl::ParseCommandLineOptions(argc, argv);

    if (helpOpt) {
        cl::PrintHelpMessage();
        return EXIT_SUCCESS;
    }

    sbt::g_debug = debugOpt;

    const sbt::Constants& c = sbt::Constants::global();

    // debug test
    if (testOpt) {
        test();
    // translate
    } else {
        if (inputFiles.empty()) {
            llvm::errs() << c.BIN_NAME << ": no input files\n";
            return EXIT_FAILURE;
        }
    }

    // -regs
    sbt::Options::Regs regs;
    if (regsOpt == "globals")
        regs = sbt::Options::Regs::GLOBALS;
    else if (regsOpt == "locals")
        regs = sbt::Options::Regs::LOCALS;
    else {
        llvm::errs() << c.BIN_NAME << ": invalid -regs value\n";
        return EXIT_FAILURE;
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
    sbt::Options opts(regs, !dontUseLibCOpt, std::atol(stackSizeOpt.c_str()));
    opts.setSyncFRegs(!dontSyncFRegsOpt)
        .setA2S(a2sOpt);
    auto exp = sbt::create<sbt::SBT>(inputFiles, outputFile, opts);
    if (!exp)
        sbt::handleError(exp.takeError());
    sbt::SBT& sbt = exp.get();

    bool hasErrors = sbt::handleError(sbt.run());

    // dump resulting IR
    // sbt.dump();

    // write IR to output file
    sbt.write();

    return hasErrors? EXIT_FAILURE : EXIT_SUCCESS;
}


/*
// generate hello world IR (for test only)
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
*/

