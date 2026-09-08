#include <stdafx.h>

#include <CDownloadTask.h>
#include <CDownloadTaskManager.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <sstream>
#include <string>

namespace {

// 在 %TEMP% 下生成一个唯一的临时文件路径，并在析构时删除该文件。
class 临时文件 {
public:
    explicit 临时文件(const std::string& suffix) {
        char dir[MAX_PATH] = {0};
        GetTempPathA(MAX_PATH, dir);

        static int counter = 0;
        std::ostringstream oss;
        oss << dir << "curldl_test_" << GetCurrentProcessId() << "_" << counter++
            << suffix;
        m_path = oss.str();
    }

    ~临时文件() { remove(m_path.c_str()); }

    const char* c_str() const { return m_path.c_str(); }
    const std::string& str() const { return m_path; }

    bool exists() const {
        DWORD attrs = GetFileAttributesA(m_path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES;
    }

private:
    std::string m_path;
};

// 一个接到临时路径上的管理器，任务直接注入，因此无需任何网络访问。
class 下载任务管理器测试 : public ::testing::Test {
protected:
    下载任务管理器测试()
        : m_xml(".xml"),
          m_local(".bin"),
          m_manager("http://example.invalid/file.bin", m_local.c_str(),
                    m_xml.c_str()) {}

    void TearDown() override { m_manager.clearDownloadTask(); }

    // 追加一个按 1 MB 对齐的分片，id 为从 1 开始的编号。
    CDownloadTask* 添加分片(int id, long long start, long long end,
                            long long downloaded, int status) {
        auto* task = new CDownloadTask;
        task->m_nTaskId = id;
        task->m_nTaskStatus = status;
        task->m_llStartPos = start;
        task->m_llEndPos = end;
        task->m_llDownloadedPos = downloaded;
        task->m_strRemotePath = m_manager.m_strRemotePath;
        task->m_strLocalPath = m_manager.m_strLocalPath;
        m_manager.pushOneTask(task);
        return task;
    }

    // 排布 count 个连续的 1 MB 分片，总长度为 contentLength 字节，与
    // generateDownloadTask() 产生的布局一致。
    void 排布分片(int count, long long contentLength, int status) {
        m_manager.m_nTasksCount = count;
        m_manager.m_llContentLength = contentLength;
        for (int i = 1; i <= count; ++i) {
            long long start = static_cast<long long>(MEGABYTES) * (i - 1);
            long long end = (i == count) ? contentLength
                                         : static_cast<long long>(MEGABYTES) * i - 1;
            long long downloaded = (status == TASK_COMPLETE) ? end : start;
            添加分片(i, start, end, downloaded, status);
        }
    }

    临时文件 m_xml;
    临时文件 m_local;
    CDownloadTaskManager m_manager;
};

// ---------------------------------------------------------------------------
// 构造
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 构造时保存路径并将计数器清零) {
    EXPECT_EQ("http://example.invalid/file.bin", m_manager.m_strRemotePath);
    EXPECT_EQ(m_local.str(), m_manager.m_strLocalPath);
    EXPECT_EQ(m_xml.str(), m_manager.m_strTaskInfoFilePath);
    EXPECT_EQ(0LL, m_manager.m_llContentLength);
    EXPECT_EQ(0LL, m_manager.m_llDownloadTime);
    EXPECT_EQ(0, m_manager.m_nTasksCount);
}

// ---------------------------------------------------------------------------
// convertLLContentLengthToString
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 转换长度为零得到空字符串) {
    EXPECT_EQ("", m_manager.convertLLContentLengthToString(0));
}

TEST_F(下载任务管理器测试, 转换长度仅字节) {
    EXPECT_EQ("512B", m_manager.convertLLContentLengthToString(512));
}

TEST_F(下载任务管理器测试, 转换长度整千字节) {
    // 2048 恰好为 2K，余数为 0，因此不带末尾的 B 部分
    EXPECT_EQ("2K", m_manager.convertLLContentLengthToString(2048));
}

TEST_F(下载任务管理器测试, 转换长度千字节带余数) {
    EXPECT_EQ("2K512B", m_manager.convertLLContentLengthToString(2048 + 512));
}

TEST_F(下载任务管理器测试, 转换长度兆字节) {
    EXPECT_EQ("5M", m_manager.convertLLContentLengthToString(5LL * MEGABYTES));
}

TEST_F(下载任务管理器测试, 转换长度兆字节带千字节) {
    long long value = 5LL * MEGABYTES + 3LL * KILOBYTES;
    EXPECT_EQ("5M3K", m_manager.convertLLContentLengthToString(value));
}

TEST_F(下载任务管理器测试, 转换长度吉字节) {
    long long value = 2LL * GIGABYTES + 100LL * MEGABYTES;
    EXPECT_EQ("2G100M", m_manager.convertLLContentLengthToString(value));
}

// ---------------------------------------------------------------------------
// convertToAboutContentLength
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 近似转换零为零字节) {
    EXPECT_EQ("0.00B", m_manager.convertToAboutContentLength(0LL));
}

TEST_F(下载任务管理器测试, 近似转换字节) {
    EXPECT_EQ("512B", m_manager.convertToAboutContentLength(512LL));
}

TEST_F(下载任务管理器测试, 近似转换兆字节取整) {
    // 5 MB + 512 KB => "5M512K" => 5 + 512/1024 = 5.5M
    long long value = 5LL * MEGABYTES + 512LL * KILOBYTES;
    EXPECT_EQ("5.5M", m_manager.convertToAboutContentLength(value));
}

TEST_F(下载任务管理器测试, 近似转换吉字节取整) {
    // 2 GB + 512 MB => 2.5G
    long long value = 2LL * GIGABYTES + 512LL * MEGABYTES;
    EXPECT_EQ("2.5G", m_manager.convertToAboutContentLength(value));
}

TEST_F(下载任务管理器测试, 近似转换字符串重载处理未知单位) {
    EXPECT_EQ("0.00B", m_manager.convertToAboutContentLength(std::string("")));
}

TEST_F(下载任务管理器测试, 近似转换字符串重载千字节) {
    EXPECT_EQ("4.5K", m_manager.convertToAboutContentLength(std::string("4K512B")));
}

// ---------------------------------------------------------------------------
// isTaskInfoFileExisted
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 任务信息文件初始不存在) {
    EXPECT_EQ(0, m_manager.isTaskInfoFileExisted());
}

TEST_F(下载任务管理器测试, 写入后能检测到任务信息文件) {
    排布分片(2, 2LL * MEGABYTES, TASK_TODO);
    ASSERT_EQ(1, m_manager.writeToFile());
    EXPECT_EQ(1, m_manager.isTaskInfoFileExisted());
}

// ---------------------------------------------------------------------------
// isTasksFinished / getTotalDownloadedLength
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 无任务时任务已完成) {
    EXPECT_EQ(1, m_manager.isTasksFinished());
}

TEST_F(下载任务管理器测试, 有待完成分片时任务未完成) {
    排布分片(3, 3LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(0, m_manager.isTasksFinished());
}

TEST_F(下载任务管理器测试, 全部分片完成时任务已完成) {
    排布分片(3, 3LL * MEGABYTES, TASK_COMPLETE);
    EXPECT_EQ(1, m_manager.isTasksFinished());
}

TEST_F(下载任务管理器测试, 无任务时已下载总长度为零) {
    EXPECT_EQ(0LL, m_manager.getTotalDownloadedLength());
}

// 记录当前实现中的差一错误：每个分片贡献 (DownloadedPos - StartPos + 1)，
// 因此 N 个尚未开始的分片就已经报告下载了 N 字节。
TEST_F(下载任务管理器测试, 已下载总长度每个分片多算一字节) {
    排布分片(4, 4LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(4LL, m_manager.getTotalDownloadedLength());
}

TEST_F(下载任务管理器测试, 已下载总长度累计进度) {
    m_manager.m_nTasksCount = 2;
    m_manager.m_llContentLength = 2LL * MEGABYTES;
    添加分片(1, 0, MEGABYTES - 1, 1000, TASK_DOING);
    添加分片(2, MEGABYTES, 2LL * MEGABYTES, MEGABYTES + 500, TASK_DOING);

    // (1000 - 0 + 1) + (1048576 + 500 - 1048576 + 1) = 1001 + 501
    EXPECT_EQ(1502LL, m_manager.getTotalDownloadedLength());
}

// ---------------------------------------------------------------------------
// freshDownloadTime
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 刷新下载时间会累加) {
    EXPECT_EQ(100LL, m_manager.freshDownloadTime(100));
    EXPECT_EQ(350LL, m_manager.freshDownloadTime(250));
    EXPECT_EQ(350LL, m_manager.m_llDownloadTime);
}

// ---------------------------------------------------------------------------
// clearDownloadTask
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 清空下载任务复位状态与任务) {
    排布分片(3, 3LL * MEGABYTES, TASK_TODO);
    m_manager.freshDownloadTime(500);
    ASSERT_FALSE(m_manager.m_vecPTasks.empty());

    EXPECT_EQ(1, m_manager.clearDownloadTask());

    EXPECT_EQ(0LL, m_manager.m_llContentLength);
    EXPECT_EQ(0LL, m_manager.m_llDownloadTime);
    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
}

// ---------------------------------------------------------------------------
// writeToFile / loadTaskFromFile 往返
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 写入文件生成可读的XML) {
    排布分片(2, 2LL * MEGABYTES, TASK_TODO);
    m_manager.freshDownloadTime(1234);

    ASSERT_EQ(1, m_manager.writeToFile());
    ASSERT_TRUE(m_xml.exists());

    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.LoadFile(m_xml.c_str()));

    auto* root = doc.RootElement();
    ASSERT_NE(nullptr, root);
    EXPECT_STREQ("TaskInfo", root->Value());

    int tasksCount = 0;
    ASSERT_EQ(tinyxml2::XML_SUCCESS,
              root->FirstChildElement("TasksCount")->QueryIntText(&tasksCount));
    EXPECT_EQ(2, tasksCount);

    int64_t contentLength = 0;
    ASSERT_EQ(tinyxml2::XML_SUCCESS,
              root->FirstChildElement("ContentLength")->QueryInt64Text(&contentLength));
    EXPECT_EQ(2LL * MEGABYTES, contentLength);

    int64_t downloadTime = 0;
    ASSERT_EQ(tinyxml2::XML_SUCCESS,
              root->FirstChildElement("DownloadTime")->QueryInt64Text(&downloadTime));
    EXPECT_EQ(1234, downloadTime);

    int taskNodes = 0;
    for (auto* e = root->FirstChildElement("Task"); e != nullptr;
         e = e->NextSiblingElement("Task")) {
        ++taskNodes;
    }
    EXPECT_EQ(2, taskNodes);
}

TEST_F(下载任务管理器测试, 从文件加载恢复每个字段) {
    排布分片(3, 3LL * MEGABYTES, TASK_TODO);
    m_manager.freshDownloadTime(777);
    // 给中间分片设置一部分进度，验证它能被保留下来。
    static_cast<CDownloadTask*>(m_manager.m_vecPTasks[1])->m_llDownloadedPos =
        MEGABYTES + 4096;
    ASSERT_EQ(1, m_manager.writeToFile());

    CDownloadTaskManager reloaded("http://example.invalid/file.bin",
                                  m_local.c_str(), m_xml.c_str());
    ASSERT_EQ(1, reloaded.loadTaskFromFile());

    EXPECT_EQ(3, reloaded.m_nTasksCount);
    EXPECT_EQ(3LL * MEGABYTES, reloaded.m_llContentLength);
    EXPECT_EQ(777LL, reloaded.m_llDownloadTime);
    ASSERT_EQ(3u, reloaded.m_vecPTasks.size());

    for (size_t i = 0; i < reloaded.m_vecPTasks.size(); ++i) {
        auto* expected = static_cast<CDownloadTask*>(m_manager.m_vecPTasks[i]);
        auto* actual = static_cast<CDownloadTask*>(reloaded.m_vecPTasks[i]);

        EXPECT_EQ(expected->m_nTaskId, actual->m_nTaskId);
        EXPECT_EQ(expected->m_nTaskStatus, actual->m_nTaskStatus);
        EXPECT_EQ(expected->m_llStartPos, actual->m_llStartPos);
        EXPECT_EQ(expected->m_llEndPos, actual->m_llEndPos);
        EXPECT_EQ(expected->m_llDownloadedPos, actual->m_llDownloadedPos);
        EXPECT_EQ(expected->m_strRemotePath, actual->m_strRemotePath);
        EXPECT_EQ(expected->m_strLocalPath, actual->m_strLocalPath);
    }

    reloaded.clearDownloadTask();
}

TEST_F(下载任务管理器测试, 从文件加载把已完成分片路由到完成队列) {
    排布分片(2, 2LL * MEGABYTES, TASK_COMPLETE);
    ASSERT_EQ(1, m_manager.writeToFile());

    CDownloadTaskManager reloaded("http://example.invalid/file.bin",
                                  m_local.c_str(), m_xml.c_str());
    ASSERT_EQ(1, reloaded.loadTaskFromFile());

    EXPECT_TRUE(reloaded.m_vecPTodoTasks.empty());
    EXPECT_EQ(2u, reloaded.m_vecPCompleteTasks.size());
    EXPECT_EQ(1, reloaded.isTasksFinished());

    reloaded.clearDownloadTask();
}

// ---------------------------------------------------------------------------
// checkTaskInfo —— 分片布局校验
//
// checkTaskInfo() 首先会把 m_llContentLength 与一次实时 HEAD 请求的结果比较。
// 这些测试覆盖的是该约定中可离线的部分：当主机不可达时 getContentLength()
// 返回 0，因此一个记录长度非零的管理器必须在任何布局校验之前就报告
// `inconsisdent`。
// ---------------------------------------------------------------------------

TEST_F(下载任务管理器测试, 校验任务信息拒绝不匹配的内容长度) {
    排布分片(2, 2LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(inconsisdent, m_manager.checkTaskInfo());
}

TEST_F(下载任务管理器测试, 校验任务信息检测错误的起始位置) {
    // 内容长度为 0，与主机不可达时的结果一致，因此下面受测的是布局校验。
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 1;
    // 编号 1 的任务必须从偏移 0 开始；4096 违反了该规则。
    添加分片(1, 4096, 8192, 4096, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

TEST_F(下载任务管理器测试, 校验任务信息检测不连续的分片) {
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 2;
    添加分片(1, 0, MEGABYTES - 1, 0, TASK_TODO);
    // 分片 2 从正确的 1 MB 边界开始，但分片 1 的结束位置没有到达该边界。
    static_cast<CDownloadTask*>(m_manager.m_vecPTasks[0])->m_llEndPos =
        MEGABYTES - 100;
    添加分片(2, MEGABYTES, 2LL * MEGABYTES - 1, MEGABYTES, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

TEST_F(下载任务管理器测试, 校验任务信息检测错误的末尾结束位置) {
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 1;
    // 最后一个分片必须满足 EndPos + 1 == ContentLength + 1，即 EndPos == 0。
    添加分片(1, 0, MEGABYTES - 1, 0, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

// ---------------------------------------------------------------------------
// errorcode 枚举
// ---------------------------------------------------------------------------

TEST(错误码测试, 取值符合文档约定) {
    EXPECT_EQ(1, noerror);
    EXPECT_EQ(2, inconsisdent);
    EXPECT_EQ(3, dividerror);
    EXPECT_EQ(4, downloadederror);
    EXPECT_EQ(5, filerror);
    EXPECT_EQ(6, remotefilerror);
    EXPECT_EQ(7, localfilerror);
}

TEST(大小宏测试, 单位为二进制倍数) {
    EXPECT_EQ(1024, KILOBYTES);
    EXPECT_EQ(1048576, MEGABYTES);
    EXPECT_EQ(1073741824, GIGABYTES);
}

}  // namespace
