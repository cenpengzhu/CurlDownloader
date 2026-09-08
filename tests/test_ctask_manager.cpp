#include <stdafx.h>

#include <CTask.h>
#include <CTaskManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

// CTaskManager::clearTask() deletes every task it holds in m_vecPTasks, so the
// fixture must only free the tasks it allocated that never reached the manager.
class CTaskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 1; i <= 3; ++i) {
            auto* t = new CTask;
            t->m_nTaskId = i;
            t->m_nTaskStatus = TASK_TODO;
            m_tasks.push_back(t);
        }
    }

    void TearDown() override {
        std::vector<CTask*> owned = m_manager.m_vecPTasks;
        m_manager.clearTask();
        for (auto* t : m_tasks) {
            if (std::find(owned.begin(), owned.end(), t) == owned.end()) {
                delete t;
            }
        }
        m_tasks.clear();
    }

    CTaskManager m_manager;
    std::vector<CTask*> m_tasks;
};

TEST_F(CTaskManagerTest, InitiallyEmpty) {
    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_EQ(0, m_manager.m_nCompleteTasksCount);
    EXPECT_EQ(0, m_manager.m_nTodoTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

TEST_F(CTaskManagerTest, PushOneTodoTask) {
    auto* task = m_tasks[0];
    EXPECT_EQ(1, m_manager.pushOneTodoTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(task, m_manager.m_vecPTodoTasks.back());
}

TEST_F(CTaskManagerTest, PushOneCompleteTask) {
    auto* task = m_tasks[0];
    task->m_nTaskStatus = TASK_COMPLETE;
    EXPECT_EQ(1, m_manager.pushOneCompleteTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(CTaskManagerTest, PushOneTaskRoutesTodo) {
    auto* task = m_tasks[0];
    ASSERT_EQ(1, m_manager.pushOneTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(0u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(CTaskManagerTest, PushOneTaskRoutesComplete) {
    auto* task = m_tasks[0];
    task->m_nTaskStatus = TASK_COMPLETE;
    ASSERT_EQ(1, m_manager.pushOneTask(task));
    EXPECT_EQ(0u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(CTaskManagerTest, PopOneTaskTodoReturnsTodoAndMarksDoing) {
    auto* task = m_tasks[0];
    m_manager.pushOneTask(task);

    CTask* popped = m_manager.popOneTaskTodo();
    ASSERT_NE(nullptr, popped);
    EXPECT_EQ(task, popped);
    EXPECT_EQ(TASK_DOING, popped->m_nTaskStatus);
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(CTaskManagerTest, PopOneTaskTodoReturnsNullOnEmpty) {
    EXPECT_EQ(nullptr, m_manager.popOneTaskTodo());
}

TEST_F(CTaskManagerTest, HaveTasksTodo) {
    EXPECT_EQ(0, m_manager.haveTasksTodo());
    m_manager.pushOneTodoTask(m_tasks[0]);
    EXPECT_EQ(1, m_manager.haveTasksTodo());
}

// collectTasks() scans m_vecPCompleteTasks — which holds both finished and
// in-flight tasks — and returns anything that is neither COMPLETE nor DOING
// (i.e. a failed TASK_DONE) to the todo queue.
TEST_F(CTaskManagerTest, CollectTasksReturnsFailedTasksToTodo) {
    auto* completed = m_tasks[0]; completed->m_nTaskStatus = TASK_COMPLETE;
    auto* failed = m_tasks[1];    failed->m_nTaskStatus = TASK_DONE;
    auto* inFlight = m_tasks[2];  inFlight->m_nTaskStatus = TASK_DOING;

    m_manager.m_vecPTasks.push_back(completed);
    m_manager.m_vecPTasks.push_back(failed);
    m_manager.m_vecPTasks.push_back(inFlight);
    m_manager.m_vecPCompleteTasks.push_back(completed);
    m_manager.m_vecPCompleteTasks.push_back(failed);
    m_manager.m_vecPCompleteTasks.push_back(inFlight);

    EXPECT_EQ(1, m_manager.collectTasks());

    ASSERT_EQ(1u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(failed, m_manager.m_vecPTodoTasks.back());

    ASSERT_EQ(2u, m_manager.m_vecPCompleteTasks.size());
    EXPECT_EQ(completed, m_manager.m_vecPCompleteTasks[0]);
    EXPECT_EQ(inFlight, m_manager.m_vecPCompleteTasks[1]);
}

TEST_F(CTaskManagerTest, CollectTasksLeavesCompleteAndDoingInPlace) {
    auto* completed = m_tasks[0]; completed->m_nTaskStatus = TASK_COMPLETE;
    auto* inFlight = m_tasks[1];  inFlight->m_nTaskStatus = TASK_DOING;

    m_manager.m_vecPTasks.push_back(completed);
    m_manager.m_vecPTasks.push_back(inFlight);
    m_manager.m_vecPCompleteTasks.push_back(completed);
    m_manager.m_vecPCompleteTasks.push_back(inFlight);

    m_manager.collectTasks();

    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_EQ(2u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(CTaskManagerTest, ClearTaskDeletesAllAndResets) {
    m_manager.pushOneTask(m_tasks[0]);
    m_manager.pushOneTask(m_tasks[1]);

    m_manager.clearTask();

    // The two pushed tasks were deleted by clearTask(); drop the dangling
    // pointers so TearDown() does not touch them again.
    m_tasks.erase(m_tasks.begin(), m_tasks.begin() + 2);

    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

TEST_F(CTaskManagerTest, HaveTasksNotCompleteWithTodo) {
    m_manager.pushOneTodoTask(m_tasks[0]);
    EXPECT_EQ(1, m_manager.haveTasksNotComplete());
}

TEST_F(CTaskManagerTest, HaveTasksNotCompleteWithFailed) {
    auto* t = m_tasks[0];
    t->m_nTaskStatus = TASK_DONE;
    m_manager.pushOneCompleteTask(t);
    EXPECT_EQ(1, m_manager.haveTasksNotComplete());
}

TEST_F(CTaskManagerTest, HaveTasksNotCompleteFalseWhenAllComplete) {
    auto* t = m_tasks[0];
    t->m_nTaskStatus = TASK_COMPLETE;
    m_manager.pushOneCompleteTask(t);
    EXPECT_EQ(0, m_manager.haveTasksNotComplete());
}

TEST_F(CTaskManagerTest, PushNullDoesNothing) {
    EXPECT_EQ(0, m_manager.pushOneTask(nullptr));
    EXPECT_EQ(0, m_manager.pushOneTodoTask(nullptr));
    EXPECT_EQ(0, m_manager.pushOneCompleteTask(nullptr));
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

}  // namespace