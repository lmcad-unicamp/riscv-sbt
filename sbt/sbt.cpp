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
            SBTError serr(file);
            serr << "no such file.";
            err = error(serr);
            return;
        }
        _translator->addInputFile(file);
    }
}


llvm::Error SBT::run()
{
    if (auto err = _translator->translate())
        return err;

    // check if generated bitcode is valid
    // (this outputs very helpful messages about the invalid bitcode parts)
    if (llvm::verifyModule(*_module, &DBGS)) {
        InvalidBitcode serr;
        serr << "translation produced invalid bitcode!";
        // _module->dump();
        return error(serr);
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


llvm::Error SBT::genSCHandler()
{
    if (auto err = _translator->genSCHandler())
        return err;
    write();
    return llvm::Error::success();
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

    // -debug is used by LLVM already
    cl::opt<bool> debugOpt("x");

    // parse args
    cl::ParseCommandLineOptions(argc, argv);

    sbt::g_debug = debugOpt;

    const sbt::Constants& c = sbt::Constants::global();

    // gen syscall handlers
    if (genSCHandlerOpt) {
        if (outputFileOpt.empty()) {
            llvm::errs() << c.BIN_NAME
                << ": output file not specified\n";
            return 1;
        }
    // debug test
    } else if (testOpt) {
        test();
    // translate
    } else {
        if (inputFiles.empty()) {
            llvm::errs() << c.BIN_NAME << ": no input files\n";
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

    bool hasErrors = false;
    // generate syscall handler
    if (genSCHandlerOpt) {
        sbt::handleError(sbt.genSCHandler());
        return EXIT_SUCCESS;
    // translate files
    } else
        hasErrors = sbt::handleError(sbt.run());

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

