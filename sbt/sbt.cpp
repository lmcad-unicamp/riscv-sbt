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
    WriteBitcodeToFile(*_module, os);
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
            serr.log(LOGS);
            gotErrors = true;
            // do not exit, let the invalid bitcode be written to make
            // debugging it easier
        },
        [](const SBTError& serr) {
            serr.log(LOGS);
            std::exit(EXIT_FAILURE);
        });

    // handle remaining errors
    if (err2) {
        logAllUnhandledErrors(std::move(err2), LOGS,
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
    Options opts;
    auto expObj = create<Object>(filePath, &opts);
    if (!expObj)
        handleError(expObj.takeError());
    else
        expObj.get().dump(llvm::outs());
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

    cl::opt<bool> syncOnExternalCallsOpt(
        "sync-on-external-calls",
        cl::desc("Synchronize register file on calls to external "
            "(non-translated) functions"));

    cl::opt<bool> commentedAsmOpt("commented-asm",
        cl::desc("Insert comments in final translated assembly code"));

    cl::opt<bool> noSymBoundsCheckOpt("no-sym-bounds-check",
        cl::desc("Don't check if non-external symbols are within their "
            "section bounds"));

    cl::opt<bool> enableFCSROpt("enable-fcsr",
        cl::desc("Enable FCSR register emulation"));

    cl::opt<bool> enableFCVTValidationOpt("enable-fcvt-validation",
        cl::desc("Enable input validation on fcvt instructions"));

    cl::opt<bool> softFloatABIOpt("soft-float-abi",
        cl::desc("Use soft-float ABI"));

    cl::opt<std::string> logFileOpt("log", cl::desc("Log file path"));

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
        .setA2S(a2sOpt)
        .setSyncOnExternalCalls(syncOnExternalCallsOpt)
        .setCommentedAsm(commentedAsmOpt)
        .setSymBoundsCheck(!noSymBoundsCheckOpt)
        .setEnableFCSR(enableFCSROpt)
        .setEnableFCVTValidation(enableFCVTValidationOpt)
        .setHardFloatABI(!softFloatABIOpt)
        .setLogFile(logFileOpt);

    sbt::Logger::get(opts.logFile());
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

