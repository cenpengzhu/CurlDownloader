#include <stdafx.h>

#include <CDownloadInfo.h>
#include <gtest/gtest.h>

namespace {

TEST(CDownloadInfoTest, DefaultConstructorZeroesEverything) {
    CDownloadInfo info;

    EXPECT_EQ(0u, info.m_dwTime);
    EXPECT_EQ(0LL, info.m_llTotalDownloadedLength);
    EXPECT_EQ(0LL, info.m_llTotalDownloadTime);
    EXPECT_DOUBLE_EQ(0.00, info.m_dPercent);
    EXPECT_EQ("0.0B/S", info.m_strSpeed);
    EXPECT_EQ("0.0B/S", info.m_strAverageSpeed);
}

TEST(CDownloadInfoTest, AssignmentOperatorCopiesAllFields) {
    CDownloadInfo src;
    src.m_dwTime = 123456;
    src.m_llTotalDownloadedLength = 5242880;
    src.m_dPercent = 42.5;
    src.m_strSpeed = "1.5M/S";
    src.m_llTotalDownloadTime = 4000;
    src.m_strAverageSpeed = "1.2M/S";

    CDownloadInfo dst;
    dst = src;

    EXPECT_EQ(src.m_dwTime, dst.m_dwTime);
    EXPECT_EQ(src.m_llTotalDownloadedLength, dst.m_llTotalDownloadedLength);
    EXPECT_DOUBLE_EQ(src.m_dPercent, dst.m_dPercent);
    EXPECT_EQ(src.m_strSpeed, dst.m_strSpeed);
    EXPECT_EQ(src.m_llTotalDownloadTime, dst.m_llTotalDownloadTime);
    EXPECT_EQ(src.m_strAverageSpeed, dst.m_strAverageSpeed);
}

TEST(CDownloadInfoTest, AssignmentIsSelfSafe) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 999;
    info.m_strSpeed = "9.9K/S";

    info = info;

    EXPECT_EQ(999LL, info.m_llTotalDownloadedLength);
    EXPECT_EQ("9.9K/S", info.m_strSpeed);
}

TEST(CDownloadInfoTest, GetAverageSpeedReturnsZeroWhenNoTimeElapsed) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 0;

    EXPECT_EQ(0LL, info.getAverageSpeed());
}

// getAverageSpeed() divides by the elapsed milliseconds *before* scaling by
// 1000, so the result is quantised to whole bytes-per-millisecond. These
// expectations pin down that lossy behaviour rather than the ideal value.
TEST(CDownloadInfoTest, GetAverageSpeedBytesPerSecond) {
    CDownloadInfo info;
    // 1 MB over 1000 ms: 1048576/1000 = 1048 B/ms, scaled back to 1048000 B/s.
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(1048000LL, info.getAverageSpeed());
}

TEST(CDownloadInfoTest, GetAverageSpeedHalvesOverDoubleTime) {
    CDownloadInfo info;
    // 1 MB over 2000 ms: 1048576/2000 = 524 B/ms => 524000 B/s.
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 2000;

    EXPECT_EQ(524000LL, info.getAverageSpeed());
}

TEST(CDownloadInfoTest, GetAverageSpeedIsExactWhenEvenlyDivisible) {
    CDownloadInfo info;
    // 2000 bytes over 1000 ms divides cleanly: 2 B/ms => 2000 B/s.
    info.m_llTotalDownloadedLength = 2000;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(2000LL, info.getAverageSpeed());
}

// Documents the integer-division precision loss of the current implementation:
// the division by total time happens *before* the multiplication by 1000, so
// any transfer slower than 1 byte/ms truncates to zero.
TEST(CDownloadInfoTest, GetAverageSpeedTruncatesBelowOneBytePerMillisecond) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 500;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(0LL, info.getAverageSpeed());
}

}  // namespace
