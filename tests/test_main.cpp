#include <stdafx.h>

#include <gtest/gtest.h>

// The component's easylogging++ statics live in the DLL's translation unit
// (CurlDownloader.cpp), which is not linked into the test binary. Define them
// here so that the LOG() calls inside the core sources resolve.
INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Keep the test output readable: the component logs verbosely to stdout.
    el::Configurations conf;
    conf.setToDefault();
    conf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
    conf.setGlobally(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(conf);

    return RUN_ALL_TESTS();
}
