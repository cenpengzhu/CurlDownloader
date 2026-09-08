#include <stdafx.h>

#include <CLibcurlTool.h>
#include <gtest/gtest.h>

#include <cstring>
#include <sstream>

// writeData 在 src/CLibcurlTool.cpp 中定义为普通（非静态）自由函数，是
// httpGet() 使用的 CURLOPT_WRITEFUNCTION 回调。这里声明出来，以便不发起
// 网络请求也能直接对它进行测试。
size_t writeData(void* ptr, size_t size, size_t nmemb, void* pstrstrResponseData);

namespace {

// figureError() 是一个只做日志输出的 switch 分支，始终返回 1。
// 由于日志输出已被关闭，这些测试只校验它的返回值。

TEST(CLibcurl工具测试, figureError始终返回1) {
    CLibcurlTool tool;
    EXPECT_EQ(1, tool.figureError(CURLE_OK));
    EXPECT_EQ(1, tool.figureError(CURLE_UNSUPPORTED_PROTOCOL));
    EXPECT_EQ(1, tool.figureError(CURLE_FAILED_INIT));
    EXPECT_EQ(1, tool.figureError(CURLE_URL_MALFORMAT));
    EXPECT_EQ(1, tool.figureError(CURLE_COULDNT_RESOLVE_HOST));
    EXPECT_EQ(1, tool.figureError(CURLE_COULDNT_CONNECT));
    EXPECT_EQ(1, tool.figureError(CURLE_OPERATION_TIMEDOUT));
    EXPECT_EQ(1, tool.figureError(CURLE_RANGE_ERROR));
    EXPECT_EQ(1, tool.figureError(CURLE_WRITE_ERROR));
    EXPECT_EQ(1, tool.figureError(CURLE_PARTIAL_FILE));
    EXPECT_EQ(1, tool.figureError(CURLE_HTTP_RETURNED_ERROR));
    EXPECT_EQ(1, tool.figureError(CURLE_OUT_OF_MEMORY));
    EXPECT_EQ(1, tool.figureError(static_cast<CURLcode>(999)));
}

TEST(CLibcurl工具测试, 构造时获取easy句柄) {
    CLibcurlTool tool;
    EXPECT_NE(nullptr, tool.m_pCurl);
}

TEST(写数据回调测试, 将内容写入字符串流) {
    std::stringstream ss;
    char input[] = "Hello, libcurl!";
    size_t written = writeData(input, 1, strlen(input), &ss);

    EXPECT_EQ(strlen(input), written);
    EXPECT_EQ("Hello, libcurl!", ss.str());
}

TEST(写数据回调测试, 多次调用会追加内容) {
    std::stringstream ss;
    char first[] = "HTTP/1.1 206 ";
    char second[] = "Partial Content";

    writeData(first, 1, strlen(first), &ss);
    writeData(second, 1, strlen(second), &ss);

    EXPECT_EQ("HTTP/1.1 206 Partial Content", ss.str());
}

// 记录当前回调的两个怪癖：它返回的是 `nmemb` 而非 `size * nmemb`，并且把
// 缓冲区当作以 NUL 结尾的 C 字符串处理，而没有遵守传入的长度参数。
TEST(写数据回调测试, 返回nmemb且遇NUL结束符即停止) {
    std::stringstream ss;
    char input[] = "test";
    size_t written = writeData(input, 2, 2, &ss);

    EXPECT_EQ(2u, written);
    EXPECT_EQ("test", ss.str());
}

}  // namespace
