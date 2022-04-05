#pragma once

//for the macro
#include<assistant_utility.h>
#include<chrono>
#include<list>
#include<set>
#include<mutex>
#include<thread_pool.h>
class TimerCaller;
class Timer;
using TimerTask = function<void(void)>;
using TimerAfter = function<void(const Timer* t, TimerCaller* tc)>;
class Timer {
public:
	using clock = chrono::system_clock;
	using time_point = clock::time_point;

	TimerTask task;
	TimerAfter afterTask;
	mutable TimerCaller* caller;
	time_point when;
	//support repeatation
	int repeatation = 0;
	Timer::clock::duration round = Timer::clock::duration();
	//repeat timer
	Timer(const TimerTask task,time_point when, Timer::clock::duration round,int times,const TimerAfter afterTask=nullptr):
		task(task), when(when), caller(nullptr), afterTask(afterTask),round(round),repeatation(times) {
		SetRepeat(round, times);
	}
	Timer(const TimerTask task, time_point when, TimerCaller* caller = nullptr,
		const TimerAfter  afterTask=nullptr, Timer::clock::duration round=chrono::milliseconds(0), int repeatation = 1) :
		task(task), when(when), caller(caller),afterTask(afterTask),repeatation(repeatation),round(round) { }
	void call() const {
		if (task != nullptr)
			task();
		if(afterTask!=nullptr)
			afterTask(this, this->caller);
	}
	void SetRepeat(Timer::clock::duration round, int times);
};
// CRPT based interface ,for the support of active&&shutdown and run,tick
template<typename T>
class _MachineState {
protected:
	atomic_bool _shutdown = true;
	Timer::time_point::duration interval = default_interval;
public:
	static constexpr Timer::time_point::duration default_interval = chrono::milliseconds(1);

	_MachineState(Timer::time_point::duration interval):interval(interval){}
	_MachineState(const _MachineState& m) :_shutdown(m._shutdown.load()), interval(m.interval) {}
	_MachineState(const _MachineState&& m) :_shutdown(m._shutdown.load()), interval(m.interval) {}
	bool is_actived() { 
		return !_shutdown;
	}
	// callergroup is obligated to active and shutdown caller
	virtual void active() { _shutdown = false; }
	virtual void shutdown() { _shutdown = true;	}
	// need active this before run
	virtual void run();
	//this may never call
	virtual void Tick() { static_cast<T*>(this)->Tick(); }
};
//simple TimerCaller
class TimerCaller:public _MachineState<TimerCaller> {
public:
	list<Timer> timers;
	recursive_mutex accesss_to_timers;
	//how long interval is
	Timer::time_point::duration interval;
public:
	TimerCaller(Timer::time_point::duration interval=default_interval):_MachineState(interval) {
		// sentinel
		timers.push_back(Timer({}, Timer::time_point::max(), this));
	}
	// follow default two will be delete because member mutex
	TimerCaller(const TimerCaller& tc) :
		_MachineState(*static_cast<const _MachineState*>(&tc)),
		timers(tc.timers), accesss_to_timers() {}
	void AddTimer(const Timer& t);

	void Tick() override;
	//set a end timer that will call shutdown when all the timer for now exectuted,
	//or just shut this down if there have no timer
	virtual void SetTerminationAtEnd();
};
// support caller management and 
class TimerCallerGroup :public _MachineState<TimerCallerGroup>{
	map<string, TimerCaller> callers;
	mutex access_to_callers;
public:
	TimerCallerGroup(Timer::clock::duration interval) :_MachineState(interval) {}
	TimerCallerGroup(const TimerCallerGroup& g) :
		_MachineState(*static_cast<const _MachineState*>(&g)), callers(g.callers), access_to_callers() {
	}
	//since within the same group, the interval actually doesn't matter
	auto AddCaller(string callerName,Timer::clock::duration interval=_MachineState::default_interval) { 
		lock_guard lg(access_to_callers);
		return callers.insert({ callerName,interval});
	}
	//check nullptr before use
	TimerCaller* GetCaller(string name, bool create_if_missing = false);
	void Tick() override{
		for (auto& [name, tc] : callers)
			tc.Tick();
	}
	void active()override;
	void shutdown()override;
};

template<typename T>
inline void _MachineState<T>::run() {
	while (is_actived()) {
		static_cast<T*>(this)->Tick();
		const auto now = Timer::clock::now();
		using rep = Timer::clock::rep;
		//precesion on ms
		auto nextTime = now + ceil<chrono::duration<int, ratio<1, 1000>>>(interval);
		this_thread::sleep_until(nextTime);
	}
}

class ConcTimerCaller :public TimerCaller {
public:
	template<typename ...args>
	ConcTimerCaller(int thread_count, args...ags) :tp(thread_count), TimerCaller(std::forward<args>(ags)...) {}

	void Tick()override {
		lock_guard lg(accesss_to_timers);
		for (auto it = timers.begin(); it != timers.end();) {
			const auto now = Timer::clock::now();
			//check if shutdown
			if (now >= it->when && is_actived()) {
				tp.enqueue([timer = move(*it)](){
					timer.call();
				});
				auto tmp = it++;
				timers.erase(tmp);
			}
			else
				break;
		}
	}
	//suppose once shutdown,never start again
	void shutdown()override {
		TimerCaller::shutdown();
		tp.shutdown();
	}
	int RestJobs() {
		lock_guard lg(accesss_to_timers);
		// reduce one of sentinel
		return timers.size() + tp.workerRunning() - 1;
	}
	GD::ThreadPool tp;
};