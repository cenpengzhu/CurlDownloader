#include <stdafx.h>

#include <gtest/gtest.h>

// 组件用到的 easylogging++ 静态变量定义在 DLL 的翻译单元（CurlDownloader.cpp）
// 中，而该文件不链接进测试可执行文件。这里补上定义，使核心源码里的 LOG() 调用
// 能够解析。
INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // 组件会向标准输出打印大量日志，这里关掉以保持测试输出整洁。
    el::Configurations conf;
    conf.setToDefault();
    conf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
    conf.setGlobally(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(conf);

    return RUN_ALL_TESTS();
}
