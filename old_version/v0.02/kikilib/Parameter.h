//@Author Liu Yukang 
#pragma once

#include <string>
#include <string.h>

namespace kikilib
{
	//�����б�
	namespace Parameter
	{
		//��־����
		static const std::string logName("Log.txt");

		//��־ռ���̵�����С��Byte�������ڸ�����������һ����־�ļ������¿�ʼд����ֹ���̱���
		static const int64_t maxLogDiskByte = 1073741824; //1024 * 1024 * 1024, 1GB

		//��־�е�ringbuffer�ĳ��ȣ���Ҫ��2��n����,һ��std::string��С��40�ֽڣ������ַ�������60�ֽڣ�
		//���Ը�����*100������Ϊ���ڴ������ָ���־ϵͳ�Ĵ�С����4194304��ʾ�ڴ�������־419MB�ռ�
		static const int64_t kLogBufferLen = 4194304;

		//��־�Ƿ��Զ�����ʱ���
		static const bool isAddTimestampInLog = true;

		static_assert(((kLogBufferLen > 0) && ((kLogBufferLen& (~kLogBufferLen + 1)) == kLogBufferLen)),
			"RingBuffer's size must be a positive power of 2");

		//�̳߳����̵߳�����
		constexpr static unsigned threadPoolCnt = 4;

		//�������еĳ���
		constexpr static unsigned backLog = 1024;

		//�����յ�������(Ҳ���������д��ڵ�EventService������)����Ҫ����linuxϵͳ��fd���޴��ڸ�ֵ
		constexpr static int maxEventServiceCnt = 600000;

		//SocketĬ���Ƿ���TCP_NoDelay
		constexpr static bool isNoDelay = true;

		//��ȡ��Ծ��epoll_event������ĳ�ʼ����
		static constexpr int epollEventListFirstSize = 16;

		//epoll_wait������ʱ��
		static constexpr int epollTimeOutMs = 10000;

		//SocketReader��SocketWritter�л������ĳ�ʼ��С
		static constexpr size_t bufferInitLen = 1024;

		//SocketReader��SocketWritter�еĻ�������
		//��ǰ����е�λ�ô�����buffer�ܴ�С��1 / bufMoveCriterion��
		//���Զ���ǰ����
		static constexpr size_t bufMoveCriterion = 3;

		//SocketReader��SocketWritter�еĻ�������
		//���Ѿ����ˣ�������Ҫ�����ݣ�������չbuffer��sizeΪ��ǰ��bufExpandRatio��
		static constexpr double bufExpandRatio = 1.5;
	};
}