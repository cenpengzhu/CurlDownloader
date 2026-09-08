#include <stdafx.h>

#include <CDownloadTask.h>
#include <CTask.h>
#include <gtest/gtest.h>

namespace {

TEST(任务测试, 状态宏取值互不相同) {
    EXPECT_EQ(0, TASK_TODO);
    EXPECT_EQ(1, TASK_COMPLETE);
    EXPECT_EQ(2, TASK_DONE);
    EXPECT_EQ(3, TASK_DOING);
}

TEST(任务测试, 字段可赋值) {
    CTask task;
    task.m_nTaskId = 42;
    task.m_nTaskStatus = TASK_DOING;

    EXPECT_EQ(42, task.m_nTaskId);
    EXPECT_EQ(TASK_DOING, task.m_nTaskStatus);
}

TEST(下载任务测试, 继承任务基类字段) {
    CDownloadTask task;
    task.m_nTaskId = 7;
    task.m_nTaskStatus = TASK_TODO;
    task.m_llStartPos = 6291456;   // 6 MB
    task.m_llEndPos = 7340031;     // 7 MB - 1
    task.m_llDownloadedPos = 6291456;
    task.m_strRemotePath = "http://example.com/file.bin";
    task.m_strLocalPath = "C:\\tmp\\file.bin";

    CTask* base = &task;
    EXPECT_EQ(7, base->m_nTaskId);
    EXPECT_EQ(TASK_TODO, base->m_nTaskStatus);

    EXPECT_EQ(6291456LL, task.m_llStartPos);
    EXPECT_EQ(7340031LL, task.m_llEndPos);
    EXPECT_EQ(task.m_llStartPos, task.m_llDownloadedPos);
    EXPECT_EQ("http://example.com/file.bin", task.m_strRemotePath);
}

TEST(下载任务测试, 分片跨度为一兆字节) {
    CDownloadTask task;
    task.m_llStartPos = 0;
    task.m_llEndPos = 1048575;

    EXPECT_EQ(1048576LL, task.m_llEndPos - task.m_llStartPos + 1);
}

}  // namespace
