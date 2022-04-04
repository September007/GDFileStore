#pragma once

//for the macro
#include<assistant_utility.h>
#include<chrono>
#include<list>
#include<mutex>
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
	Timer(TimerTask&& task, time_point when, TimerCaller* caller = nullptr,TimerAfter afterTask=nullptr) :
		task(task), when(when), caller(caller),afterTask(afterTask) { }
	void call() const {
		if (task != nullptr)
			task();
		if(afterTask!=nullptr)
			afterTask(this, this->caller);
	}
};
// CRPT based interface ,for the support of active&&shutdown and run,tick
template<typename T>
class _MachineState {
protected:
	atomic_bool _shutdown = true;
	Timer::time_point::duration interval = default_interval;
public:
	using Timer = Timer;
	static constexpr Timer::time_point::duration default_interval = chrono::milliseconds(1);

	_MachineState(Timer::time_point::duration interval):interval(interval){}
	_MachineState(const _MachineState& m) :_shutdown(m._shutdown.load()), interval(m.interval) {}
	_MachineState(const _MachineState&& m) :_shutdown(m._shutdown.load()), interval(m.interval) {}
	bool is_actived() { return !_shutdown.load(); }
	void active() { _shutdown = false; }
	void shutdown() { _shutdown = true; }
	//may need active this before run
	virtual void run() {
		while (is_actived()) {
			static_cast<T*>(this)->Tick();
			const auto now = Timer::clock::now();
			using rep = Timer::clock::rep;
			//precesion on ms
			auto nextTime = ceil<chrono::duration<int, ratio<1, 1000>>>(now + interval);
			this_thread::sleep_until(nextTime);
		}
	}
	//this may never call
	virtual void Tick() { static_cast<T*>(this)->Tick(); }
};
//simple TimerCaller
class TimerCaller:public _MachineState<TimerCaller> {
public:
	list<Timer> timers;
	mutex accesss_to_timers;
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
	void AddTimer(const Timer& t) {
		lock_guard lg(accesss_to_timers);
		DebugArea(int foreSize = timers.size());
		t.caller = this;
		for (auto it = timers.begin(); it != timers.end(); ++it) {
			if (it->when > t.when) {
				timers.insert(it, t);
				break;
			}
			else continue;
		}
		DebugArea(LOG_EXPECT_EQ("Timer", timers.size(), foreSize + 1));
	}

	void Tick() override {
		lock_guard lg(accesss_to_timers);
		for (auto it = timers.begin(); it != timers.end();) {
			const auto now = Timer::clock::now();
			if (now >= it->when) {
				it->call();
				auto tmp = it++;
				timers.erase(tmp);
			}
			else
				break;
		}
	}
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
	auto AddCaller(string callerName,_MachineState::Timer::clock::duration interval=_MachineState::default_interval) { 
		lock_guard lg(access_to_callers);
		return callers.insert({ callerName,interval});
	}
	//check nullptr before use
	TimerCaller* GetCaller(string name,bool create_if_missing=false){
		auto x = callers.find(name);
		if (x != callers.end())
			return &(x->second);
		else if (create_if_missing) {
			auto a= AddCaller(name);
			if (a.second)
				return &(a.first->second);
		}
		return nullptr;
	}
	void Tick() override{
		for (auto& [name, tc] : callers)
			tc.Tick();
	}
};