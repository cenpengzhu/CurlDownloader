#include <stdafx.h>

#include <CTask.h>
#include <CThread.h>
#include <CThreadManager.h>
#include <gtest/gtest.h>

namespace {

// CThread::init() spawns a real worker thread that blocks on m_hRunEvent while
// paused. These tests drive the manager's bookkeeping directly instead, so no
// threads are started and taskBusiness() is never invoked.
class CThreadManagerTest : public ::testing::Test {
protected:
    void TearDown() override {
        for (auto* t : m_owned) delete t;
        m_owned.clear();
    }

    CThread* makeThread(int status) {
        auto* t = new CThread(m_manager.m_hDownloadThreadMutex,
                              m_manager.m_hPauseEvent);
        t->m_nThreadStatus = status;
        m_owned.push_back(t);
        return t;
    }

    CThreadManager m_manager;
    std::vector<CThread*> m_owned;
};

TEST_F(CThreadManagerTest, ConstructorCreatesSyncObjects) {
    EXPECT_NE(nullptr, m_manager.m_hDownloadThreadMutex);
    EXPECT_NE(nullptr, m_manager.m_hPauseEvent);
    EXPECT_EQ(0, m_manager.m_nThreadsCount);
}

TEST_F(CThreadManagerTest, InitiallyNoThreadsFreeOrBusy) {
    EXPECT_EQ(0, m_manager.haveThreadsFree());
    EXPECT_EQ(0, m_manager.haveThreadsRun());
}

TEST_F(CThreadManagerTest, HaveThreadsFreeReflectsPool) {
    m_manager.m_vecFreeThreads.push_back(makeThread(THREAD_PAUSE));
    EXPECT_EQ(1, m_manager.haveThreadsFree());
    EXPECT_EQ(0, m_manager.haveThreadsRun());
}

TEST_F(CThreadManagerTest, HaveThreadsRunReflectsBusyPool) {
    m_manager.m_vecBusyThreads.push_back(makeThread(THREAD_RUN));
    EXPECT_EQ(1, m_manager.haveThreadsRun());
    EXPECT_EQ(0, m_manager.haveThreadsFree());
}

TEST_F(CThreadManagerTest, GetOneFreeThreadsPopsFromPool) {
    auto* a = makeThread(THREAD_PAUSE);
    auto* b = makeThread(THREAD_PAUSE);
    m_manager.m_vecFreeThreads.push_back(a);
    m_manager.m_vecFreeThreads.push_back(b);

    // Pool is LIFO.
    EXPECT_EQ(b, m_manager.getOneFreeThreads());
    EXPECT_EQ(a, m_manager.getOneFreeThreads());
    EXPECT_EQ(nullptr, m_manager.getOneFreeThreads());
}

TEST_F(CThreadManagerTest, GetOneFreeThreadsReturnsNullWhenExhausted) {
    EXPECT_EQ(nullptr, m_manager.getOneFreeThreads());
}

TEST_F(CThreadManagerTest, StopThreadsMarksFreeThreadsStopped) {
    auto* a = makeThread(THREAD_PAUSE);
    auto* b = makeThread(THREAD_PAUSE);
    m_manager.m_vecFreeThreads.push_back(a);
    m_manager.m_vecFreeThreads.push_back(b);

    EXPECT_EQ(1, m_manager.stopThreads());
    EXPECT_EQ(THREAD_STOP, a->m_nThreadStatus.load());
    EXPECT_EQ(THREAD_STOP, b->m_nThreadStatus.load());
}

TEST_F(CThreadManagerTest, CollectThreadsMovesPausedBackToFreePool) {
    auto* running = makeThread(THREAD_RUN);
    auto* paused = makeThread(THREAD_PAUSE);
    m_manager.m_vecBusyThreads.push_back(running);
    m_manager.m_vecBusyThreads.push_back(paused);

    EXPECT_EQ(1, m_manager.collectThreads());

    ASSERT_EQ(1u, m_manager.m_vecBusyThreads.size());
    EXPECT_EQ(running, m_manager.m_vecBusyThreads.front());
    ASSERT_EQ(1u, m_manager.m_vecFreeThreads.size());
    EXPECT_EQ(paused, m_manager.m_vecFreeThreads.front());
}

TEST_F(CThreadManagerTest, CollectThreadsIsNoOpWhenAllBusy) {
    m_manager.m_vecBusyThreads.push_back(makeThread(THREAD_RUN));
    m_manager.collectThreads();

    EXPECT_EQ(1u, m_manager.m_vecBusyThreads.size());
    EXPECT_TRUE(m_manager.m_vecFreeThreads.empty());
}

TEST_F(CThreadManagerTest, GiveTaskToAThreadFailsWithoutFreeThread) {
    CTask task;
    task.m_nTaskId = 1;
    task.m_nTaskStatus = TASK_TODO;

    EXPECT_EQ(0, m_manager.giveTaskToAThreadTodo(&task));
    EXPECT_TRUE(m_manager.m_vecBusyThreads.empty());
}

TEST_F(CThreadManagerTest, GiveTaskToAThreadAssignsAndMovesToBusy) {
    auto* thread = makeThread(THREAD_PAUSE);
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

TEST(CThreadTest, StatusMacrosAreDistinct) {
    EXPECT_EQ(0, THREAD_STOP);
    EXPECT_EQ(1, THREAD_RUN);
    EXPECT_EQ(2, THREAD_PAUSE);
}

TEST(CThreadTest, ConstructorStartsPaused) {
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

TEST(CThreadTest, SetTaskStoresPointerWithoutRunning) {
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

TEST(CThreadTest, RunFlipsStatusToRunning) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    CThread thread(mutex, pauseEvent);

    EXPECT_EQ(1, thread.Run());
    EXPECT_EQ(THREAD_RUN, thread.m_nThreadStatus.load());

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

TEST(CThreadTest, BaseTaskBusinessIsANoOp) {
    HANDLE mutex = CreateMutexW(NULL, FALSE, NULL);
    HANDLE pauseEvent = CreateEvent(NULL, 0, 1, NULL);
    CThread thread(mutex, pauseEvent);

    EXPECT_EQ(1, thread.taskBusiness());

    CloseHandle(thread.m_hRunEvent);
    CloseHandle(pauseEvent);
    CloseHandle(mutex);
}

}  // namespace
