//@Author Liu Yukang 
#pragma once

#include <string>
#include <string.h>

namespace kikilib
{
	//参数列表
	namespace Parameter
	{
		//日志名字
		static const std::string logName("Log.txt");

		//日志占磁盘的最大大小（Byte），大于该数会舍弃掉一个日志文件，重新开始写，防止磁盘爆满
		static const int64_t maxLogDiskByte = 1073741824; //1024 * 1024 * 1024, 1GB

		//日志中的ringbuffer的长度，需要是2的n次幂,一个std::string大小是40字节，加上字符串假设60字节，
		//所以该数字*100可以认为是内存允许分给日志系统的大小，如4194304表示内存最多给日志419MB空间
		static const int64_t kLogBufferLen = 4194304;

		//日志是否自动添加时间戳
		static const bool isAddTimestampInLog = true;

		static_assert(((kLogBufferLen > 0) && ((kLogBufferLen& (~kLogBufferLen + 1)) == kLogBufferLen)),
			"RingBuffer's size must be a positive power of 2");

		//线程池中线程的数量
		constexpr static unsigned threadPoolCnt = 4;

		//监听队列的长度
		constexpr static unsigned backLog = 8192;

		//最大接收的连接数(也即服务器中存在的EventService对象数)，需要设置linux系统的fd上限大于该值
		constexpr static int maxEventServiceCnt = 600000;

		//Socket默认是否开启TCP_NoDelay
		constexpr static bool isNoDelay = true;

		//获取活跃的epoll_event的数组的初始长度
		static constexpr int epollEventListFirstSize = 16;

		//epoll_wait的阻塞时长
		static constexpr int epollTimeOutMs = 10000;

		//SocketReader和SocketWritter中缓冲区的初始大小
		static constexpr size_t bufferInitLen = 1024;

		//SocketReader和SocketWritter中的缓冲区，
		//若前面空闲的位置大于了buffer总大小的1 / bufMoveCriterion，
		//则自动向前补齐
		static constexpr size_t bufMoveCriterion = 3;

		//SocketReader和SocketWritter中的缓冲区，
		//若已经满了，但是需要读数据，会先扩展buffer的size为当前的bufExpandRatio倍
		static constexpr double bufExpandRatio = 1.5;

		//内存池没有空闲内存块时申请memPoolMallocObjCnt个对象大小的内存块
		static constexpr size_t memPoolMallocObjCnt = 40;
	};
}