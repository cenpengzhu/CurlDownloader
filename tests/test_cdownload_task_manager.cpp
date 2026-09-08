#include <stdafx.h>

#include <CDownloadTask.h>
#include <CDownloadTaskManager.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <sstream>
#include <string>

namespace {

// Builds a unique scratch path under %TEMP% and removes it on destruction.
class TempFile {
public:
    explicit TempFile(const std::string& suffix) {
        char dir[MAX_PATH] = {0};
        GetTempPathA(MAX_PATH, dir);

        static int counter = 0;
        std::ostringstream oss;
        oss << dir << "curldl_test_" << GetCurrentProcessId() << "_" << counter++
            << suffix;
        m_path = oss.str();
    }

    ~TempFile() { remove(m_path.c_str()); }

    const char* c_str() const { return m_path.c_str(); }
    const std::string& str() const { return m_path; }

    bool exists() const {
        DWORD attrs = GetFileAttributesA(m_path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES;
    }

private:
    std::string m_path;
};

// A manager wired to temp paths, with tasks injected directly so that no
// network access is required.
class CDownloadTaskManagerTest : public ::testing::Test {
protected:
    CDownloadTaskManagerTest()
        : m_xml(".xml"),
          m_local(".bin"),
          m_manager("http://example.invalid/file.bin", m_local.c_str(),
                    m_xml.c_str()) {}

    void TearDown() override { m_manager.clearDownloadTask(); }

    // Appends a 1 MB-aligned slice with the given 1-based id.
    CDownloadTask* addSlice(int id, long long start, long long end,
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

    // Lays out `count` contiguous 1 MB slices totalling `contentLength` bytes,
    // matching what generateDownloadTask() would produce.
    void layOutSlices(int count, long long contentLength, int status) {
        m_manager.m_nTasksCount = count;
        m_manager.m_llContentLength = contentLength;
        for (int i = 1; i <= count; ++i) {
            long long start = static_cast<long long>(MEGABYTES) * (i - 1);
            long long end = (i == count) ? contentLength
                                         : static_cast<long long>(MEGABYTES) * i - 1;
            long long downloaded = (status == TASK_COMPLETE) ? end : start;
            addSlice(i, start, end, downloaded, status);
        }
    }

    TempFile m_xml;
    TempFile m_local;
    CDownloadTaskManager m_manager;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, ConstructorStoresPathsAndZeroesCounters) {
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

TEST_F(CDownloadTaskManagerTest, ConvertLengthZeroYieldsEmptyString) {
    EXPECT_EQ("", m_manager.convertLLContentLengthToString(0));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthBytesOnly) {
    EXPECT_EQ("512B", m_manager.convertLLContentLengthToString(512));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthKilobytes) {
    // 2048 = 2K exactly, remainder 0 so no trailing B component
    EXPECT_EQ("2K", m_manager.convertLLContentLengthToString(2048));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthKilobytesWithRemainder) {
    EXPECT_EQ("2K512B", m_manager.convertLLContentLengthToString(2048 + 512));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthMegabytes) {
    EXPECT_EQ("5M", m_manager.convertLLContentLengthToString(5LL * MEGABYTES));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthMegabytesWithKilobytes) {
    long long value = 5LL * MEGABYTES + 3LL * KILOBYTES;
    EXPECT_EQ("5M3K", m_manager.convertLLContentLengthToString(value));
}

TEST_F(CDownloadTaskManagerTest, ConvertLengthGigabytes) {
    long long value = 2LL * GIGABYTES + 100LL * MEGABYTES;
    EXPECT_EQ("2G100M", m_manager.convertLLContentLengthToString(value));
}

// ---------------------------------------------------------------------------
// convertToAboutContentLength
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, ConvertAboutZeroIsZeroBytes) {
    EXPECT_EQ("0.00B", m_manager.convertToAboutContentLength(0LL));
}

TEST_F(CDownloadTaskManagerTest, ConvertAboutBytes) {
    EXPECT_EQ("512B", m_manager.convertToAboutContentLength(512LL));
}

TEST_F(CDownloadTaskManagerTest, ConvertAboutRoundsMegabytes) {
    // 5 MB + 512 KB => "5M512K" => 5 + 512/1024 = 5.5M
    long long value = 5LL * MEGABYTES + 512LL * KILOBYTES;
    EXPECT_EQ("5.5M", m_manager.convertToAboutContentLength(value));
}

TEST_F(CDownloadTaskManagerTest, ConvertAboutRoundsGigabytes) {
    // 2 GB + 512 MB => 2.5G
    long long value = 2LL * GIGABYTES + 512LL * MEGABYTES;
    EXPECT_EQ("2.5G", m_manager.convertToAboutContentLength(value));
}

TEST_F(CDownloadTaskManagerTest, ConvertAboutStringOverloadHandlesUnknownUnit) {
    EXPECT_EQ("0.00B", m_manager.convertToAboutContentLength(std::string("")));
}

TEST_F(CDownloadTaskManagerTest, ConvertAboutStringOverloadKilobytes) {
    EXPECT_EQ("4.5K", m_manager.convertToAboutContentLength(std::string("4K512B")));
}

// ---------------------------------------------------------------------------
// isTaskInfoFileExisted
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, TaskInfoFileMissingInitially) {
    EXPECT_EQ(0, m_manager.isTaskInfoFileExisted());
}

TEST_F(CDownloadTaskManagerTest, TaskInfoFileDetectedAfterWrite) {
    layOutSlices(2, 2LL * MEGABYTES, TASK_TODO);
    ASSERT_EQ(1, m_manager.writeToFile());
    EXPECT_EQ(1, m_manager.isTaskInfoFileExisted());
}

// ---------------------------------------------------------------------------
// isTasksFinished / getTotalDownloadedLength
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, TasksFinishedTrueWhenNoTasks) {
    EXPECT_EQ(1, m_manager.isTasksFinished());
}

TEST_F(CDownloadTaskManagerTest, TasksFinishedFalseWithPendingSlice) {
    layOutSlices(3, 3LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(0, m_manager.isTasksFinished());
}

TEST_F(CDownloadTaskManagerTest, TasksFinishedTrueWhenAllComplete) {
    layOutSlices(3, 3LL * MEGABYTES, TASK_COMPLETE);
    EXPECT_EQ(1, m_manager.isTasksFinished());
}

TEST_F(CDownloadTaskManagerTest, TotalDownloadedLengthZeroWithoutTasks) {
    EXPECT_EQ(0LL, m_manager.getTotalDownloadedLength());
}

// Documents the off-by-one in the current implementation: each slice
// contributes (DownloadedPos - StartPos + 1), so N untouched slices already
// report N bytes downloaded.
TEST_F(CDownloadTaskManagerTest, TotalDownloadedLengthCountsOneExtraBytePerSlice) {
    layOutSlices(4, 4LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(4LL, m_manager.getTotalDownloadedLength());
}

TEST_F(CDownloadTaskManagerTest, TotalDownloadedLengthAccumulatesProgress) {
    m_manager.m_nTasksCount = 2;
    m_manager.m_llContentLength = 2LL * MEGABYTES;
    addSlice(1, 0, MEGABYTES - 1, 1000, TASK_DOING);
    addSlice(2, MEGABYTES, 2LL * MEGABYTES, MEGABYTES + 500, TASK_DOING);

    // (1000 - 0 + 1) + (1048576 + 500 - 1048576 + 1) = 1001 + 501
    EXPECT_EQ(1502LL, m_manager.getTotalDownloadedLength());
}

// ---------------------------------------------------------------------------
// freshDownloadTime
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, FreshDownloadTimeAccumulates) {
    EXPECT_EQ(100LL, m_manager.freshDownloadTime(100));
    EXPECT_EQ(350LL, m_manager.freshDownloadTime(250));
    EXPECT_EQ(350LL, m_manager.m_llDownloadTime);
}

// ---------------------------------------------------------------------------
// clearDownloadTask
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, ClearDownloadTaskResetsStateAndTasks) {
    layOutSlices(3, 3LL * MEGABYTES, TASK_TODO);
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
// writeToFile / loadTaskFromFile round-trip
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, WriteToFileProducesReadableXml) {
    layOutSlices(2, 2LL * MEGABYTES, TASK_TODO);
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

TEST_F(CDownloadTaskManagerTest, LoadTaskFromFileRestoresEveryField) {
    layOutSlices(3, 3LL * MEGABYTES, TASK_TODO);
    m_manager.freshDownloadTime(777);
    // Give the middle slice some partial progress to verify it survives.
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

TEST_F(CDownloadTaskManagerTest, LoadTaskFromFileRoutesCompletedSlices) {
    layOutSlices(2, 2LL * MEGABYTES, TASK_COMPLETE);
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
// checkTaskInfo — slice-layout validation
//
// checkTaskInfo() first compares m_llContentLength against a live HEAD request.
// These tests exercise the offline half of the contract: with an unreachable
// host getContentLength() returns 0, so a manager whose recorded length is
// non-zero must report `inconsisdent` before any layout check runs.
// ---------------------------------------------------------------------------

TEST_F(CDownloadTaskManagerTest, CheckTaskInfoRejectsMismatchedContentLength) {
    layOutSlices(2, 2LL * MEGABYTES, TASK_TODO);
    EXPECT_EQ(inconsisdent, m_manager.checkTaskInfo());
}

TEST_F(CDownloadTaskManagerTest, CheckTaskInfoDetectsBadStartPosition) {
    // Content length 0 matches the unreachable-host result, so the layout
    // checks below are the ones under test.
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 1;
    // TaskId 1 must start at offset 0; 4096 violates the rule.
    addSlice(1, 4096, 8192, 4096, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

TEST_F(CDownloadTaskManagerTest, CheckTaskInfoDetectsNonContiguousSlices) {
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 2;
    addSlice(1, 0, MEGABYTES - 1, 0, TASK_TODO);
    // Slice 2 starts at the right 1 MB boundary but slice 1 ends short of it.
    static_cast<CDownloadTask*>(m_manager.m_vecPTasks[0])->m_llEndPos =
        MEGABYTES - 100;
    addSlice(2, MEGABYTES, 2LL * MEGABYTES - 1, MEGABYTES, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

TEST_F(CDownloadTaskManagerTest, CheckTaskInfoDetectsWrongFinalEndPosition) {
    m_manager.m_llContentLength = 0;
    m_manager.m_nTasksCount = 1;
    // Last slice must satisfy EndPos + 1 == ContentLength + 1, i.e. EndPos == 0.
    addSlice(1, 0, MEGABYTES - 1, 0, TASK_TODO);

    EXPECT_EQ(dividerror, m_manager.checkTaskInfo());
}

// ---------------------------------------------------------------------------
// errorcode enum
// ---------------------------------------------------------------------------

TEST(ErrorCodeTest, ValuesMatchDocumentedContract) {
    EXPECT_EQ(1, noerror);
    EXPECT_EQ(2, inconsisdent);
    EXPECT_EQ(3, dividerror);
    EXPECT_EQ(4, downloadederror);
    EXPECT_EQ(5, filerror);
    EXPECT_EQ(6, remotefilerror);
    EXPECT_EQ(7, localfilerror);
}

TEST(SizeMacroTest, UnitsAreBinaryMultiples) {
    EXPECT_EQ(1024, KILOBYTES);
    EXPECT_EQ(1048576, MEGABYTES);
    EXPECT_EQ(1073741824, GIGABYTES);
}

}  // namespace
