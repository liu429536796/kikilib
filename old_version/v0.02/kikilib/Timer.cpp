//@Author Liu Yukang 
#include "Timer.h"
#include "LogManager.h"

#include <sys/timerfd.h>
#include <string.h>

using namespace kikilib;

void Timer::GetExpiredTask(std::vector<std::function<void()>>& tasks)
{
	Time nowTime = Time::now();
	for(auto it = _timerCbMap.begin(); it != _timerCbMap.end() && it->first <= nowTime; it = _timerCbMap.begin())
	{
		tasks.emplace_back(std::move(it->second));
		_timerCbMap.erase(it);
	}
	
	if (!_timerCbMap.empty())
	{
		auto it = _timerCbMap.begin();
		ResetTimeOfTimefd(it->first);
	}
}

void Timer::RunAt(Time time, std::function<void()>& cb)
{
	if (_timerCbMap.find(time) != _timerCbMap.end())
	{//ͨ����1΢������ͻ
		return RunAt(time.GetTimeVal() + 1, cb);
	}
	_timerCbMap.insert(std::move(std::pair<Time, std::function<void()>>(time, cb)));
	if (_timerCbMap.begin()->first == time)
	{//�¼�������������������������Ҫ����timefd�����õ�ʱ��
		ResetTimeOfTimefd(time);
	}
}

void Timer::RunAt(Time time, std::function<void()>&& cb)
{
	if (_timerCbMap.find(time) != _timerCbMap.end())
	{//ͨ����1΢������ͻ
		return RunAt(time.GetTimeVal() + 1, std::move(cb));
	}
	_timerCbMap.insert(std::move(std::pair<Time, std::function<void()>>(time, std::move(cb))));
	if (_timerCbMap.begin()->first == time)
	{//�¼�������������������������Ҫ����timefd�����õ�ʱ��
		ResetTimeOfTimefd(time);
	}
}

//��timefd��������ʱ�䣬time�Ǿ���ʱ��
void Timer::ResetTimeOfTimefd(Time time)
{
	struct itimerspec newValue;
	struct itimerspec oldValue;
	memset(&newValue, 0, sizeof newValue);
	memset(&oldValue, 0, sizeof oldValue);
	newValue.it_value = time.TimeIntervalFromNow();
	int ret = ::timerfd_settime(_timeSock.fd(), 0, &newValue, &oldValue);
	if (ret)
	{
		RecordLog(ERROR_DATA_INFORMATION, std::string("timerfd_settime failed. errno : ") + std::to_string(errno));
	}
}
