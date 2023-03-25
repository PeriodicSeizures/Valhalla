// main.cpp
#define SOL_ALL_SAFETIES_ON 1

#include <csignal>
#include <optick.h>
#include <easylogging++.h>

#include "VUtils.h"
#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"

#include "Tests.h"

INITIALIZE_EASYLOGGINGPP

// https://stackoverflow.com/a/17350413
// Colors for output
#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define GOLD "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GRAY "\033[90m"

void initLogger(bool colors) {
    el::Configurations loggerConfiguration;

    // https://github.com/amrayn/easyloggingpp#datetime-format-specifiers
    // https://github.com/amrayn/easyloggingpp/blob/master/README.md#using-configuration-file
    // [%fbase:L%line]
    //std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, VALHALLA_LOGFILE_NAME);

    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, (colors ? GOLD : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format,
        (colors ? GOLD : "") + std::string("[%datetime{%H:%m:%s}] [%thread thread] %fbase:L%line: %msg") + (colors ? RESET : ""));

    loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, format);

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    if (colors)
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

    LOG(INFO) << "Logger is configured";
}

// I've violated several visibility modifier practices regarding
//  valhalla having access to init/uninit methods of managers, and no otherclasses...
//  so is this really reasonable>
//class ValhallaLauncher {
//public:
//    void Launch() {
//        Valhalla()->Start();
//    }
//};

// Steam Documentation https://partner.steamgames.com/doc/sdk/api
int main(int argc, char **argv) {
    OPTICK_THREAD("main");
    

#ifdef RUN_TESTS
    fs::current_path("./data/tests/");

    Tests().Test_ZStdCompressorDecompressor();

    //Tests().Test_FileWriteLines();

    //Tests().Test_ResourceReadBytes();
    //Tests().Test_ResourceLines();

    //Tests().Test_FileWriteBytes();

    //Tests().Test_ParentChildTransforms();

    //Tests().Test_DungeonGenerator();

    //Tests().Test_RectInsideRect();
    //Tests().Test_RectOverlap();

    //Tests().Test_LinesIntersect();

    //Tests().Test_ParentChildTransforms();
    //Tests::Test_QuaternionLook();

    //Tests::Test_PeerLuaConnect();
    //Tests::Test_DataBuffer();
    //Tests::Test_World();
    //Tests().Test_ZDO();
    //Tests::Test_ResourceReadWrite();
    //Tests::Test_Random();
    //Tests::Test_Perlin();

    LOG(INFO) << "All tests passed!";

    return 0;
#endif



    const char* root = "./data/";
    bool colors = true;
    bool backup_logs = true;
    for (int i = 1; i < argc-1; i++) {
        if (strcmp(argv[i], "-root") == 0) {
            if (i < argc) root = argv[++i];
        }
        else if (strcmp(argv[i], "-colors") == 0) {
            if (i < argc) colors = strcmp(argv[++i], "false") == 0 || strcmp(argv[i], "0") == 0 ? false : true;
        }
        else if (strcmp(argv[i], "-backup-logs") == 0) {
            if (i < argc) backup_logs = strcmp(argv[++i], "false") == 0 || strcmp(argv[i], "0") == 0 ? false : true;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            if (i < argc) el::Loggers::setVerboseLevel(std::stoi(argv[++i]));
        }
    }

    fs::current_path(root);

    initLogger(colors);

    // Copy any old file
    if (backup_logs) {
        std::error_code ec;
        fs::create_directories("logs", ec);

        if (auto log = VUtils::Resource::ReadFile<BYTES_t>(VALHALLA_LOGFILE_NAME)) {
            if (auto opt = GZCompressor().Compress(*log))
                VUtils::Resource::WriteFile(
                    fs::path("logs") / (std::to_string(steady_clock::now().time_since_epoch().count()) + "-" + VALHALLA_LOGFILE_NAME + ".gz"), 
                    *opt);
        }
        //fs::copy_file(VALHALLA_LOGFILE_NAME,
            //std::to_string(steady_clock::now().time_since_epoch().count()) + VALHALLA_LOGFILE_NAME, ec);
    }
    
#ifndef _DEBUG
    try {
#endif
        //ValhallaLauncher().Launch();
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
#endif

    return 0;
}
