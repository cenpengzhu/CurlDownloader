#include <stdafx.h>

#include <CTask.h>
#include <CThread.h>
#include <CThreadManager.h>
#include <gtest/gtest.h>

namespace {

// CThread::init() 会真正创建一个工作线程，暂停时阻塞在 m_hRunEvent 上。
// 这些测试直接驱动管理器的内部记账逻辑，因此不会启动任何线程，也不会调用
// taskBusiness()。
class 线程管理器测试 : public ::testing::Test {
protected:
    void TearDown() override {
        for (auto* t : m_已持有线程) delete t;
        m_已持有线程.clear();
    }

    CThread* 创建线程(int status) {
        auto* t = new CThread(m_manager.m_hDownloadThreadMutex,
                              m_manager.m_hPauseEvent);
        t->m_nThreadStatus = status;
        m_已持有线程.push_back(t);
        return t;
    }

    CThreadManager m_manager;
    std::vector<CThread*> m_已持有线程;
};

TEST_F(线程管理器测试, 构造时创建同步对象) {
    EXPECT_NE(nullptr, m_manager.m_hDownloadThreadMutex);
    EXPECT_NE(nullptr, m_manager.m_hPauseEvent);
    EXPECT_EQ(0, m_manager.m_nThreadsCount);
}

TEST_F(线程管理器测试, 初始时空闲与忙碌线程均为零) {
    EXPECT_EQ(0, m_manager.haveThreadsFree());
    EXPECT_EQ(0, m_manager.haveThreadsRun());
}

TEST_F(线程管理器测试, 空闲线程数反映空闲池) {
    m_manager.m_vecFreeThreads.push_back(创建线程(THREAD_PAUSE));
    EXPECT_EQ(1, m_manager.haveThreadsFree());
    EXPECT_EQ(0, m_manager.haveThreadsRun());
}

TEST_F(线程管理器测试, 忙碌线程数反映忙碌池) {
    m_manager.m_vecBusyThreads.push_back(创建线程(THREAD_RUN));
    EXPECT_EQ(1, m_manager.haveThreadsRun());
    EXPECT_EQ(0, m_manager.haveThreadsFree());
}

TEST_F(线程管理器测试, 取一个空闲线程从池中弹出) {
    auto* a = 创建线程(THREAD_PAUSE);
    auto* b = 创建线程(THREAD_PAUSE);
    m_manager.m_vecFreeThreads.push_back(a);
    m_manager.m_vecFreeThreads.push_back(b);

    // 线程池是后进先出（LIFO）。
    EXPECT_EQ(b, m_manager.getOneFreeThreads());
    EXPECT_EQ(a, m_manager.getOneFreeThreads());
    EXPECT_EQ(nullptr, m_manager.getOneFreeThreads());
}

TEST_F(线程管理器测试, 空闲线程耗尽时返回空) {
    EXPECT_EQ(nullptr, m_manager.getOneFreeThreads());
}

TEST_F(线程管理器测试, 停止线程会把空闲线程标记为停止) {
    auto* a = 创建线程(THREAD_PAUSE);
    auto* b = 创建线程(THREAD_PAUSE);
    m_manager.m_vecFreeThreads.push_back(a);
    m_manager.m_vecFreeThreads.push_back(b);

    EXPECT_EQ(1, m_manager.stopThreads());
    EXPECT_EQ(THREAD_STOP, a->m_nThreadStatus.load());
    EXPECT_EQ(THREAD_STOP, b->m_nThreadStatus.load());
}

TEST_F(线程管理器测试, 回收线程把已暂停线程移回空闲池) {
    auto* running = 创建线程(THREAD_RUN);
    auto* paused = 创建线程(THREAD_PAUSE);
    m_manager.m_vecBusyThreads.push_back(running);
    m_manager.m_vecBusyThreads.push_back(paused);

    EXPECT_EQ(1, m_manager.collectThreads());

    ASSERT_EQ(1u, m_manager.m_vecBusyThreads.size());
    EXPECT_EQ(running, m_manager.m_vecBusyThreads.front());
    ASSERT_EQ(1u, m_manager.m_vecFreeThreads.size());
    EXPECT_EQ(paused, m_manager.m_vecFreeThreads.front());
}

TEST_F(线程管理器测试, 全部忙碌时回收线程为空操作) {
    m_manager.m_vecBusyThreads.push_back(创建线程(THREAD_RUN));
    m_manager.collectThreads();

    EXPECT_EQ(1u, m_manager.m_vecBusyThreads.size());
    EXPECT_TRUE(m_manager.m_vecFreeThreads.empty());
}

TEST_F(线程管理器测试, 无空闲线程时分发任务失败) {
    CTask task;
    task.m_nTaskId = 1;
    task.m_nTaskStatus = TASK_TODO;

    EXPECT_EQ(0, m_manager.giveTaskToAThreadTodo(&task));
    EXPECT_TRUE(m_manager.m_vecBusyThreads.empty());
}

TEST_F(线程管理器测试, 分发任务会绑定并移入忙碌池) {
    auto* thread = 创建线程(THREAD_PAUSE);
    m_manager.m_vecFreeThreads.push_back(thread);

    CTask task;
    task.m_nTaskId = 9;
    task.m_nTaskStatus = TASK_DOING;

    EXPECT_EQ(1, m_manager.giveTaskToAThreadTodo(&task));

    EXPECT_EQ(&task, thread->m_pThreadTask);
    EXPECT_EQ(THREAD_RUN, thread->m_nThreadStatus.load());
    EXPECT_TRUE(m_manager.m_vecFreeThreads.empty());
    ASSERT_EQ(1u, m_manager.m_vecBusyThreads.size());
    EXPECT_EQ(thread, m_manager.m_vecBusyThreads.front());
}

// ---------------------------------------------------------------------------
// CThread
// ---------------------------------------------------------------------------

TEST(线程测试, 状态宏取值互不相同) {
    EXPECT_EQ(0, THREAD_STOP);
    EXPECT_EQ(1, THREAD_RUN);
    EXPECT_EQ(2, THREAD_PAUSE);
}

TEST(线程测试, 构造后处于暂停状态) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    ASSERT_NE(nullptr, mutex);
    ASSERT_NE(nullptr, pauseEvent);

    CThread thread(mutex, pauseEvent);
    EXPECT_EQ(THREAD_PAUSE, thread.m_nThreadStatus.load());
    EXPECT_EQ(mutex, thread.m_hDownloadThreadMutex);
    EXPECT_EQ(pauseEvent, thread.m_hPauseEvent);
    EXPECT_NE(nullptr, thread.m_hRunEvent);

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

TEST(线程测试, 设置任务只保存指针而不运行) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    CThread thread(mutex, pauseEvent);

    CTask task;
    task.m_nTaskId = 3;

    EXPECT_EQ(1, thread.setTask(&task));
    EXPECT_EQ(&task, thread.m_pThreadTask);
    EXPECT_EQ(THREAD_PAUSE, thread.m_nThreadStatus.load());

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

TEST(线程测试, 运行后状态翻转为运行中) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    CThread thread(mutex, pauseEvent);

    EXPECT_EQ(1, thread.Run());
    EXPECT_EQ(THREAD_RUN, thread.m_nThreadStatus.load());

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

TEST(线程测试, 基类任务处理为空操作) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    CThread thread(mutex, pauseEvent);

    EXPECT_EQ(1, thread.taskBusiness());

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

}  // namespace
