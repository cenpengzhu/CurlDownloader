#include <stdafx.h>

#include <CTask.h>
#include <CTaskManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

// CTaskManager::clearTask() 会 delete 掉 m_vecPTasks 中持有的每一个任务，
// 因此夹具只能释放那些由它分配、但从未交给管理器的任务。
class 任务管理器测试 : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 1; i <= 3; ++i) {
            auto* t = new CTask;
            t->m_nTaskId = i;
            t->m_nTaskStatus = TASK_TODO;
            m_任务.push_back(t);
        }
    }

    void TearDown() override {
        std::vector<CTask*> 已交给管理器 = m_manager.m_vecPTasks;
        m_manager.clearTask();
        for (auto* t : m_任务) {
            if (std::find(已交给管理器.begin(), 已交给管理器.end(), t)
                == 已交给管理器.end()) {
                delete t;
            }
        }
        m_任务.clear();
    }

    CTaskManager m_manager;
    std::vector<CTask*> m_任务;
};

TEST_F(任务管理器测试, 初始为空) {
    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_EQ(0, m_manager.m_nCompleteTasksCount);
    EXPECT_EQ(0, m_manager.m_nTodoTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

TEST_F(任务管理器测试, 压入一个待办任务) {
    auto* task = m_任务[0];
    EXPECT_EQ(1, m_manager.pushOneTodoTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(task, m_manager.m_vecPTodoTasks.back());
}

TEST_F(任务管理器测试, 压入一个完成任务) {
    auto* task = m_任务[0];
    task->m_nTaskStatus = TASK_COMPLETE;
    EXPECT_EQ(1, m_manager.pushOneCompleteTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(任务管理器测试, 压入任务按待办路由) {
    auto* task = m_任务[0];
    ASSERT_EQ(1, m_manager.pushOneTask(task));
    EXPECT_EQ(1u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(0u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(任务管理器测试, 压入任务按完成路由) {
    auto* task = m_任务[0];
    task->m_nTaskStatus = TASK_COMPLETE;
    ASSERT_EQ(1, m_manager.pushOneTask(task));
    EXPECT_EQ(0u, m_manager.m_vecPTodoTasks.size());
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(任务管理器测试, 弹出待办任务返回该任务并标记为进行中) {
    auto* task = m_任务[0];
    m_manager.pushOneTask(task);

    CTask* popped = m_manager.popOneTaskTodo();
    ASSERT_NE(nullptr, popped);
    EXPECT_EQ(task, popped);
    EXPECT_EQ(TASK_DOING, popped->m_nTaskStatus);
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_EQ(1u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(任务管理器测试, 无待办任务时弹出返回空) {
    EXPECT_EQ(nullptr, m_manager.popOneTaskTodo());
}

TEST_F(任务管理器测试, 查询是否有待办任务) {
    EXPECT_EQ(0, m_manager.haveTasksTodo());
    m_manager.pushOneTodoTask(m_任务[0]);
    EXPECT_EQ(1, m_manager.haveTasksTodo());
}

// collectTasks() 会扫描 m_vecPCompleteTasks —— 其中同时存放着已完成和进行中的
// 任务 —— 并把既不是 COMPLETE 也不是 DOING 的任务（即失败的 TASK_DONE）退回
// 待办队列。
TEST_F(任务管理器测试, 回收任务把失败任务退回待办) {
    auto* completed = m_任务[0]; completed->m_nTaskStatus = TASK_COMPLETE;
    auto* failed = m_任务[1];    failed->m_nTaskStatus = TASK_DONE;
    auto* inFlight = m_任务[2];  inFlight->m_nTaskStatus = TASK_DOING;

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

TEST_F(任务管理器测试, 回收任务保留已完成和进行中的任务) {
    auto* completed = m_任务[0]; completed->m_nTaskStatus = TASK_COMPLETE;
    auto* inFlight = m_任务[1];  inFlight->m_nTaskStatus = TASK_DOING;

    m_manager.m_vecPTasks.push_back(completed);
    m_manager.m_vecPTasks.push_back(inFlight);
    m_manager.m_vecPCompleteTasks.push_back(completed);
    m_manager.m_vecPCompleteTasks.push_back(inFlight);

    m_manager.collectTasks();

    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_EQ(2u, m_manager.m_vecPCompleteTasks.size());
}

TEST_F(任务管理器测试, 清空任务会删除全部并复位) {
    m_manager.pushOneTask(m_任务[0]);
    m_manager.pushOneTask(m_任务[1]);

    m_manager.clearTask();

    // 压入的两个任务已被 clearTask() 删除；这里丢掉悬垂指针，避免
    // TearDown() 再次触碰它们。
    m_任务.erase(m_任务.begin(), m_任务.begin() + 2);

    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

TEST_F(任务管理器测试, 有待办任务时存在未完成任务) {
    m_manager.pushOneTodoTask(m_任务[0]);
    EXPECT_EQ(1, m_manager.haveTasksNotComplete());
}

TEST_F(任务管理器测试, 有失败任务时存在未完成任务) {
    auto* t = m_任务[0];
    t->m_nTaskStatus = TASK_DONE;
    m_manager.pushOneCompleteTask(t);
    EXPECT_EQ(1, m_manager.haveTasksNotComplete());
}

TEST_F(任务管理器测试, 全部完成时不存在未完成任务) {
    auto* t = m_任务[0];
    t->m_nTaskStatus = TASK_COMPLETE;
    m_manager.pushOneCompleteTask(t);
    EXPECT_EQ(0, m_manager.haveTasksNotComplete());
}

TEST_F(任务管理器测试, 压入空指针不做任何事) {
    EXPECT_EQ(0, m_manager.pushOneTask(nullptr));
    EXPECT_EQ(0, m_manager.pushOneTodoTask(nullptr));
    EXPECT_EQ(0, m_manager.pushOneCompleteTask(nullptr));
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPTodoTasks.empty());
    EXPECT_TRUE(m_manager.m_vecPCompleteTasks.empty());
}

}  // namespace
