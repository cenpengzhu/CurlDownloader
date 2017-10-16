#pragma once

//任务状态宏
#define TASK_TODO  0     //待处理
#define TASK_COMPLETE 1  //已完成
#define TASK_DONE 2      //失败
#define TASK_DOING 3     //正在处理

class CTask {
public:
	//任务状态
	int m_nTaskStatus;
	//任务编号
	int m_nTaskId;
};