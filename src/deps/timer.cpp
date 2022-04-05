#include<timer.h>

void Timer::SetRepeat(Timer::clock::duration round, int times) {
	if (times <= 1)return;
	this->repeatation = times;
	auto tmp = this->afterTask;
	this->afterTask = [=,tmp=move(tmp)](const Timer* t, TimerCaller* tc) {
		if (tmp != nullptr)
			tmp(t, tc);
		if (tc && t && t->repeatation > 1) {
			tc->AddTimer(Timer(t->task, t->when + round, t->caller,t->afterTask,t->round, t->repeatation - 1));
		}
	};
}

void TimerCaller::AddTimer(const Timer& t) {
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

void TimerCaller::Tick() {
	lock_guard lg(accesss_to_timers);
	for (auto it = timers.begin(); it != timers.end();) {
		const auto now = Timer::clock::now();
		//check if shutdown
		if (now >= it->when &&is_actived()) {
			it->call();
			auto tmp = it++;
			timers.erase(tmp);
		}
		else
			break;
	}
}

void TimerCaller::SetTerminationAtEnd() {
	lock_guard lg(accesss_to_timers);
	if (timers.size() == 1)
		this->shutdown();
	else {
		auto end = ----timers.end();
		AddTimer(Timer([]() {}, end->when, nullptr, [](const Timer*, TimerCaller* tc) {
			tc->shutdown();
			}));
	}
}

TimerCaller* TimerCallerGroup::GetCaller(string name, bool create_if_missing) {
	auto x = callers.find(name);
	if (x != callers.end())
		return &(x->second);
	else if (create_if_missing) {
		auto a = AddCaller(name);
		if (a.second)
			return &(a.first->second);
	}
	return nullptr;
}

void TimerCallerGroup::active() {
	this->_shutdown = false;
	for (auto& [name, caller] : callers)
		caller.active();
}

void TimerCallerGroup::shutdown() {
	this->_shutdown = true;
	for (auto& [name, caller] : callers)
		caller.shutdown();
}
