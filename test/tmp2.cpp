#define head TMP2
#include<test_head.h>
#include<fmt/format.h>
#include <iostream>
#include<timer.h>
using namespace std::chrono;
//TEST(TimerCaller, x) {
//	auto now = Timer::clock::now();
//	cout << fmt::format("{:8}\n", "add ims");
//	for (int i = 0; i < 1000; i += 10) {
//		auto n = now + chrono::milliseconds(i);
//		auto n_s = chrono::time_point_cast<duration<int, ratio<1, 1>>>(n).time_since_epoch().count();
//		auto n_ms = chrono::time_point_cast<duration<int, ratio<1, 1'000>>>(n).time_since_epoch().count();
//		auto n_ns = chrono::time_point_cast<duration<int, ratio<1, 1'000'000>>>(n).time_since_epoch().count();
//		cout << fmt::format("{:8} {:5}s {:5}ms {:5}ns\n", i,n_s, n_ms, n_ns);
//	}
//}
//
TEST(TimerCaller,basic) {
	set<int> s;
	s.insert({ 1,2,3,4 });
	TimerCaller tc;
	auto now = Timer::clock::now();
	auto sinceNow = chrono::milliseconds(0);
	constexpr auto tries = 10000;
	for (int i = tries-1; i>=0; --i) {
		tc.AddTimer(Timer([i, now, &tc,&sinceNow]() {
			auto since = Timer::clock::now() - now;
			EXPECT_TRUE(since > sinceNow);
			sinceNow = chrono::duration_cast<chrono::milliseconds> (since);
			if (i == tries-1)
				tc.shutdown();
			}
		, now + std::chrono::microseconds(10 + i*10)));
	}
	tc.active();
	this_thread::sleep_for(chrono::milliseconds(100));
	tc.run();
	int cnt = 100, repeats = 100, timerTask = 0;
	for (int i = 0; i < cnt; ++i) {
		tc.AddTimer(Timer([i,cnt,&tc,&timerTask]() {
			timerTask++;
			}, Timer::clock::now() + chrono::milliseconds(1), chrono::milliseconds(1), repeats,
				[i,cnt](const Timer* t, TimerCaller* ptc) {
				if (i == cnt - 1 && t->repeatation == 1)
					ptc->shutdown();
			}));
	}
	tc.active();
	tc.run();
	EXPECT_EQ(timerTask, cnt * repeats);
}

//TEST(TimerCaller, overheadOfNow) {
//	int tries = 100000;
//	vector<Timer::clock::time_point> tps(tries);
//	auto t0 = Timer::clock::now();
//	for (auto& t : tps)
//		t = Timer::clock::now();
//	auto t1 = Timer::clock::now();
//	cout <<tries<<"call now cost "<< chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() <<"ms" << endl;
//	auto x = Timer::clock::now();
//}
//rough
TEST(TimerCaller, thread_safe) {
	// 10ms each tick
	TimerCaller tc(chrono::duration<int, ratio<1, 100>>(1));
	vector<int> out;
	auto time = chrono::milliseconds(0) + Timer::clock::now();
	auto addTimer = [&](int val) {
		tc.AddTimer(Timer([val, &out]() {
			out.push_back(val);
			},time));
	};
	tc.active(); 
	thread run([&]() {
		this_thread::sleep_for(chrono::milliseconds(10));
		tc.run(); });
	int tries = 10000, ts = tries;
	unsigned _nom = 0;
	while (tc.is_actived() && tries--) {
		addTimer(tries);
		//idle
		for (int j = 0; j < 100; _nom *= 3, ++j);
	}
	tc.SetTerminationAtEnd();
	run.join();
	for (int i = 0; i < out.size(); ++i)
		EXPECT_EQ(ts - 1 - i, out[i]);
}
TEST(TimerCallerGroup, basic) {
	TimerCallerGroup g(chrono::milliseconds(1));
	g.AddCaller("caller0");
	g.AddCaller("caller1");
	vector<int> ans0, ans1;
	int tries = 100;
	auto now = Timer::clock::now();
	for (int i = tries-1; i >= 0; --i) {
		g.GetCaller("caller0")->AddTimer(Timer([&ans0,i, &g, tries]() {
			ans0.push_back(i);
			if (i == tries-1)
				g.shutdown();
			}, now + chrono::milliseconds(10 + i)));
		g.GetCaller("caller1")->AddTimer(Timer([&ans1, i,&g,tries]() {
			ans1.push_back(i);
			if (i == tries-1)
				g.shutdown();
			}, now + chrono::milliseconds(10 + i)));
	}
	g.active();
	this_thread::sleep_for(chrono::milliseconds(30));
	g.run();
	for (int i = 0; i<int(ans0.size()); ++i)
		EXPECT_EQ(ans0[i], i);
	for (int i = 0; i<int(ans1.size()); ++i)
		EXPECT_EQ(ans1[i], i);
}
int conc = thread::hardware_concurrency();

TEST(ConcTimerCaller, all) {
	ConcTimerCaller ctc(conc);
	int tries = 10, repeat = 10;
	atomic_int cnt = 0;
	auto now = Timer::clock::now();
		
	for (int i = 0; i < tries; ++i)
		ctc.AddTimer(Timer([&cnt]() {
		cnt++;
		this_thread::sleep_for(chrono::milliseconds(1));
			}, now, chrono::milliseconds(2), repeat));
	ctc.active();
	thread shut([&ctc]() {while (ctc.RestJobs()); ctc.shutdown(); });
	ctc.run();
	shut.join();
	EXPECT_EQ(cnt, repeat * tries);
}