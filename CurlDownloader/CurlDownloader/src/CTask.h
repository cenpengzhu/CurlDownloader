#pragma once

//����״̬��
#define TASK_TODO  0     //������
#define TASK_COMPLETE 1  //�����
#define TASK_DONE 2      //ʧ��
#define TASK_DOING 3     //���ڴ���

class CTask {
public:
	//����״̬
	int m_nTaskStatus;
	//������
	int m_nTaskId;
};