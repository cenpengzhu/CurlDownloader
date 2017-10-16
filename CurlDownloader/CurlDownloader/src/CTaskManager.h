#pragma  once

#include <CTask.h>
#include <vector>

using namespace std;

class CTaskManager {
public:
	//Task数组
	vector<CTask *> m_vecPTasks;
	//Task数目
	int m_nTasksCount;

	//已完成Task数组
	vector<CTask *> m_vecPCompleteTasks;
	//已完成Task数目
	int m_nCompleteTasksCount;

	//未完成Task数组
	vector<CTask *> m_vecPTodoTasks;
	//未完成Task数目
	int m_nTodoTasksCount;

	CTaskManager(){
		m_nTasksCount = 0;
		m_nCompleteTasksCount = 0;
		m_nTodoTasksCount = 0;

	}
	

	//添加一个任务
	int pushOneTask(CTask * pTask){
		if (pTask != NULL)
		{
			m_vecPTasks.push_back(pTask);
			//m_nTasksCount = m_nTasksCount + 1;
			if (pTask->m_nTaskStatus == TASK_COMPLETE)
			{
				return pushOneCompleteTask(pTask);
			}
			else{
				return pushOneTodoTask(pTask);
			}
		}
		else{
			return 0;
		}
	}
	//取出一个待处理任务
	virtual CTask * popOneTaskTodo(){
		CTask* pTask;
		if (m_vecPTodoTasks.size() == 0)
		{
			return NULL;
		}
		else{

			pTask = m_vecPTodoTasks.back();
			m_vecPTodoTasks.pop_back();
			pTask->m_nTaskStatus = TASK_DOING;
			m_vecPCompleteTasks.push_back(pTask);
			return pTask;
		}
	}

	//添加一个待处理任务
	virtual int pushOneTodoTask(CTask * pTask){
		if (pTask != NULL)
		{
			m_vecPTodoTasks.push_back(pTask);
			return 1;
		}
		else{
			return 0;
		}
	}

	//添加一个已完成任务
	virtual int pushOneCompleteTask(CTask * pTask){
		if (pTask != NULL)
		{
			m_vecPCompleteTasks.push_back(pTask);
			return 1;
		}
		else{
			return 0;
		}
	}

	int clearTask(){
		for (vector<CTask *>::const_iterator iterPTask = m_vecPTasks.begin();iterPTask != m_vecPTasks.end();iterPTask++)
		{
			delete (*iterPTask);
		}
		m_vecPTasks.clear();
		m_vecPCompleteTasks.clear();
		m_vecPTodoTasks.clear();
		m_nTasksCount = 0;
		m_nCompleteTasksCount = 0;
		m_nTodoTasksCount = 0;

		return 1;
	}

	int haveTasksTodo(){
		if (m_vecPTodoTasks.size() != 0)
		{
			return 1;
		}
		else{
			return 0;
		}
	}

	int haveTasksNotComplete() {
		CTask * p;
		if (m_vecPTodoTasks.size() != 0)
		{
			return 1;
		}
		else {
			for (vector<CTask *>::const_iterator iterPTask = m_vecPCompleteTasks.begin();iterPTask != m_vecPCompleteTasks.end();iterPTask++)
			{
				p = *iterPTask;
				if (p->m_nTaskStatus != TASK_COMPLETE) {
					return 1;
				}
			}
			return 0;
		}
	}
	//回收任务
	int collectTasks(){
		CTask * p;
		for (vector<CTask *>::const_iterator iterPTask = m_vecPCompleteTasks.begin();iterPTask != m_vecPCompleteTasks.end();)
		{
			p = *iterPTask;
			if (p->m_nTaskStatus != TASK_COMPLETE && p->m_nTaskStatus != TASK_DOING)
			{
				iterPTask = m_vecPCompleteTasks.erase(iterPTask);
				m_vecPTodoTasks.push_back(p);
			}
			else{
				iterPTask++;
			}
		}
		return 1;
	}
};