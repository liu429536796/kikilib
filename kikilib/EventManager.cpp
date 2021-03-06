//@Author Liu Yukang 
#include "EventManager.h"
#include "LogManager.h"
#include "EventService.h"
#include "TimerEventService.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "spinlock_guard.h"

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <signal.h>

using namespace kikilib;

EventManager::EventManager(int idx, EventServicePool* evServePool, ThreadPool* threadPool)
	: _idx(idx), _quit(false), _pEvServePool(evServePool),
	_pThreadPool(threadPool),  _pLooper(nullptr), _pTimer(nullptr), _pCtx(nullptr)
{ }

EventManager::~EventManager()
{
	_quit = true;
	RecordLog(ERROR_DATA_INFORMATION, std::to_string(_idx) + " EventManager being deleted!");
	if (_pLooper)
	{
		_pLooper->join();
		delete _pLooper;
	}
	for (auto pEvServ : _eventSet)
	{
		delete pEvServ;
	}
	if (_pTimer)
	{
		delete _pTimer;
	}
}

bool EventManager::loop()
{
	if (_pThreadPool == nullptr)
	{//判断线程池工具是否有效
		RecordLog(ERROR_DATA_INFORMATION, std::to_string(_idx) + " get a null threadpool!");
		return false;
	}
	//初始化EventEpoller
	if (!_epoller.init())
	{
		RecordLog(ERROR_DATA_INFORMATION, std::to_string(_idx) + " init epoll fd failed!");
		return false;
	}
	//初始化定时器服务
	int timeFd = ::timerfd_create(CLOCK_MONOTONIC,
		TFD_NONBLOCK | TFD_CLOEXEC);
	if (timeFd < 0)
	{
		RecordLog(ERROR_DATA_INFORMATION,std::to_string(_idx) + " eventManager timer init failed!");
		return false;
	}
	Socket timeSock(timeFd);
	_pTimer = new Timer(timeSock);
	EventService* pTimerServe = new TimerEventService(timeSock, this);
	//设置定时器事件为优先级最高的事件
	pTimerServe->setEventPriority(IMMEDIATE_EVENT);

	insertEv(pTimerServe);

	//初始化loop
	_pLooper = new std::thread(
		[this]
		{
			while (!_quit)
			{
				//清空所有列表
				if(_actEvServs.size())
                {
                   _actEvServs.clear();
                }
				for (int i = 0; i < EVENT_PRIORITY_TYPE_COUNT; ++i)
				{
				    if(_priorityEvQue[i].size())
                    {
                       _priorityEvQue[i].clear();
                    }
				}
				//获取活跃事件
				_epoller.getActEvServ(Parameter::epollTimeOutMs, _actEvServs);
				//每次epoll更新一次服务器大致时间
				//Time::UpdataRoughTime();
				//按优先级分队
				for (auto pEvServ : _actEvServs)
				{
					if (pEvServ->getEventPriority() >= IMMEDIATE_EVENT)
					{
						(_priorityEvQue[IMMEDIATE_EVENT]).push_back(pEvServ);
					}
					else
					{
						(_priorityEvQue[NORMAL_EVENT]).push_back(pEvServ);
					}
				}
				//按照优先级处理事件
				for (int i = 0; i < EVENT_PRIORITY_TYPE_COUNT; ++i)
				{
					for (auto evServ : _priorityEvQue[i])
					{
						evServ->handleEvent();
					}
				}
				//处理不再关注的事件
				{
					SpinlockGuard lock(_removedEvSpLck);

					for (auto unusedEv : _removedEv)
					{
						//从监听事件中移除
						this->_epoller.removeEv(unusedEv);
						
						{//close这个fd
							SpinlockGuard poolLock(_evPoolSpLck);
							_pEvServePool->RetrieveEventService(unusedEv);
						}
					}
					if (_removedEv.size())
					{
						_removedEv.clear();
					}
				}
				
			}
		}
		);
	return true;
}

//向事件管理器中插入一个事件
void EventManager::insertEv(EventService* ev)
{
	if (!ev)
	{
		return;
	}

	if (ev->isConnected())
	{//insert意味着连接事件的触发
		ev->handleConnectionEvent();
	}
	else
	{
		return;
	}
	
	if (ev->isConnected())
	{

		{
			SpinlockGuard lock(_evSetSpLck);
			_eventSet.insert(ev);
		}

		_epoller.addEv(ev);
	}
	
}

//向事件管理器中移除一个事件
void EventManager::removeEv(EventService* ev)
{
	if (!ev)
	{
		return;
	}
	
	{//从映射表中删除事件
		SpinlockGuard lock(_evSetSpLck);
		auto it = _eventSet.find(ev);
		if (it != _eventSet.end())
		{
			_eventSet.erase(it);
		}
	}
	
	{//放入被移除事件列表
		SpinlockGuard lock(_removedEvSpLck);
		_removedEv.push_back(ev);
	}
}

//向事件管理器中修改一个事件服务所关注的事件类型
void EventManager::modifyEv(EventService* ev)
{
	if (!ev)
	{
		return;
	}
	bool isNewEv = false;

	{
		SpinlockGuard lock(_evSetSpLck);
		if (_eventSet.find(ev) == _eventSet.end())
		{
			isNewEv = true;
		}
	}

	if (isNewEv)
	{
		insertEv(ev);
	}
	else
	{
		_epoller.modifyEv(ev);
	}
}

TimerTaskId EventManager::runAt(Time time, std::function<void()>&& timerCb)
{
	SpinlockGuard lock(_timerSpLck);
	return _pTimer->runAt(time, std::move(timerCb));
}

TimerTaskId EventManager::runAt(Time time, std::function<void()>& timerCb)
{
	SpinlockGuard lock(_timerSpLck);
	return _pTimer->runAt(time, timerCb);
}

//time时间后执行timerCb函数
TimerTaskId EventManager::runAfter(Time time, std::function<void()>&& timerCb)
{
	Time runTime(Time::now().getTimeVal() + time.getTimeVal());
	
	SpinlockGuard lock(_timerSpLck);
	return _pTimer->runAt(runTime, std::move(timerCb));
	
}

//time时间后执行timerCb函数
TimerTaskId EventManager::runAfter(Time time, std::function<void()>& timerCb)
{
	Time runTime(Time::now().getTimeVal() + time.getTimeVal());

	SpinlockGuard lock(_timerSpLck);
	return  _pTimer->runAt(runTime, timerCb);
}

//time时间后执行timerCb函数
void EventManager::runEvery(Time time, std::function<void()> timerCb, TimerTaskId& retId)
{
	//这里lamda表达式不加括号会立刻执行
	std::function<void()> realTimerCb(
		[this, time, timerCb, &retId]
		{
			timerCb();
			this->runEvery(time, timerCb, retId);
		}
		);

	retId = runAfter(time, std::move(realTimerCb));

}

//每过time时间执行一次timerCb函数,直到isContinue函数返回false
void EventManager::runEveryUntil(Time time, std::function<void()> timerCb, std::function<bool()> isContinue, TimerTaskId& retId)
{
	std::function<void()> realTimerCb(
		[this, time, timerCb, isContinue, &retId]
		{
			if (isContinue())
			{
				timerCb();
				this->runEveryUntil(time, timerCb, isContinue, retId);
			}
		}
		);

	retId = runAfter(time, std::move(realTimerCb));
}

void EventManager::removeTimerTask(TimerTaskId& timerId)
{
	_pTimer->removeTimerTask(timerId);
}

//运行所有已经超时的需要执行的函数
void EventManager::runExpired()
{
	SpinlockGuard lock(_timerQueSpLck);

	{
		SpinlockGuard lock(_timerSpLck);
		_pTimer->getExpiredTask(_actTimerTasks);
	}
	
	for (auto& task : _actTimerTasks)
	{
		task();
	}

	_actTimerTasks.clear();
}

//将任务放在线程池中以达到异步执行的效果
void EventManager::runInThreadPool(std::function<void()>&& func)
{
    _pThreadPool->enqueue(std::move(func));
}

//设置EventManager区域唯一的上下文内容
void EventManager::setEvMgrCtx(void* ctx)
{
	SpinlockGuard lock(_ctxSpLck);
	_pCtx = ctx;
}

//设置EventManager区域唯一的上下文内容
void* EventManager::getEvMgrCtx() 
{
	return _pCtx; 
}

size_t EventManager::eventServeCnt()
{
	return _eventSet.size();
}

EventService* EventManager::CreateEventService(Socket& sock)
{
	EventService* ev;

	{
		SpinlockGuard lock(_evPoolSpLck);
		ev = _pEvServePool->CreateEventService(sock, this);
	}

	return ev;
}
