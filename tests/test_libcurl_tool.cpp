#include <stdafx.h>

#include <CLibcurlTool.h>
#include <gtest/gtest.h>

#include <cstring>
#include <sstream>

// Defined in src/CLibcurlTool.cpp as a plain (non-static) free function; it is
// the CURLOPT_WRITEFUNCTION callback used by httpGet(). Declared here so it can
// be exercised without issuing a network request.
size_t writeData(void* ptr, size_t size, size_t nmemb, void* pstrstrResponseData);

namespace {

// figureError() is a pure-logging switch that always returns 1.
// These tests only verify the return value since the log output is suppressed.

TEST(CLibcurlToolTest, FigureErrorAlwaysReturnsOne) {
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

TEST(CLibcurlToolTest, ConstructorAcquiresEasyHandle) {
    CLibcurlTool tool;
    EXPECT_NE(nullptr, tool.m_pCurl);
}

TEST(WriteDataTest, WritesIntoStringStream) {
    std::stringstream ss;
    char input[] = "Hello, libcurl!";
    size_t written = writeData(input, 1, strlen(input), &ss);

    EXPECT_EQ(strlen(input), written);
    EXPECT_EQ("Hello, libcurl!", ss.str());
}

TEST(WriteDataTest, AppendsAcrossSuccessiveCalls) {
    std::stringstream ss;
    char first[] = "HTTP/1.1 206 ";
    char second[] = "Partial Content";

    writeData(first, 1, strlen(first), &ss);
    writeData(second, 1, strlen(second), &ss);

    EXPECT_EQ("HTTP/1.1 206 Partial Content", ss.str());
}

// Documents two quirks of the current callback: it returns `nmemb` rather than
// `size * nmemb`, and it treats the buffer as a NUL-terminated C string instead
// of honouring the length arguments.
TEST(WriteDataTest, ReturnsNmembAndStopsAtNulTerminator) {
    std::stringstream ss;
    char input[] = "test";
    size_t written = writeData(input, 2, 2, &ss);

    EXPECT_EQ(2u, written);
    EXPECT_EQ("test", ss.str());
}

}  // namespace