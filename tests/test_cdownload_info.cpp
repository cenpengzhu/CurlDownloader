#include <stdafx.h>

#include <CDownloadInfo.h>
#include <gtest/gtest.h>

namespace {

TEST(下载信息测试, 默认构造将各项清零) {
    CDownloadInfo info;

    EXPECT_EQ(0u, info.m_dwTime);
    EXPECT_EQ(0LL, info.m_llTotalDownloadedLength);
    EXPECT_EQ(0LL, info.m_llTotalDownloadTime);
    EXPECT_DOUBLE_EQ(0.00, info.m_dPercent);
    EXPECT_EQ("0.0B/S", info.m_strSpeed);
    EXPECT_EQ("0.0B/S", info.m_strAverageSpeed);
}

TEST(下载信息测试, 赋值运算符拷贝全部字段) {
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

TEST(下载信息测试, 自赋值安全) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 999;
    info.m_strSpeed = "9.9K/S";

    info = info;

    EXPECT_EQ(999LL, info.m_llTotalDownloadedLength);
    EXPECT_EQ("9.9K/S", info.m_strSpeed);
}

TEST(下载信息测试, 未经过时间时平均速度返回零) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 0;

    EXPECT_EQ(0LL, info.getAverageSpeed());
}

// getAverageSpeed() 先按经过的毫秒数做除法，再乘以 1000，因此结果被量化为
// 整数“字节每毫秒”。下面的断言锁定的是这种有损行为，而非理想值。
TEST(下载信息测试, 平均速度换算为字节每秒) {
    CDownloadInfo info;
    // 1000 ms 下载 1 MB：1048576/1000 = 1048 B/ms，换算回 1048000 B/s。
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(1048000LL, info.getAverageSpeed());
}

TEST(下载信息测试, 时间加倍则平均速度减半) {
    CDownloadInfo info;
    // 2000 ms 下载 1 MB：1048576/2000 = 524 B/ms => 524000 B/s。
    info.m_llTotalDownloadedLength = 1048576;
    info.m_llTotalDownloadTime = 2000;

    EXPECT_EQ(524000LL, info.getAverageSpeed());
}

TEST(下载信息测试, 可整除时平均速度精确) {
    CDownloadInfo info;
    // 1000 ms 下载 2000 字节可整除：2 B/ms => 2000 B/s。
    info.m_llTotalDownloadedLength = 2000;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(2000LL, info.getAverageSpeed());
}

// 记录当前实现中整数除法带来的精度损失：乘以 1000 之前就先按总时间做了除法，
// 因此任何低于 1 字节/毫秒 的传输速率都会被截断为零。
TEST(下载信息测试, 低于每毫秒一字节时平均速度截断为零) {
    CDownloadInfo info;
    info.m_llTotalDownloadedLength = 500;
    info.m_llTotalDownloadTime = 1000;

    EXPECT_EQ(0LL, info.getAverageSpeed());
}

}  // namespace
