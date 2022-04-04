#define head TMP2
#include<test_head.h>
#include<fmt\format.h>
#include <iostream>
#include<timer.h>
using namespace std::chrono;

TEST(TimerCaller,all) {
	TimerCaller tc;
	auto now = Timer::clock::now();
	auto sinceNow = chrono::milliseconds(0);
	constexpr auto tries = 10000;
	for (int i = tries-1; i>=0; --i) {
		tc.AddTimer(Timer([i, now, &tc,&sinceNow]() {
			auto since = TimerCaller::Timer::clock::now() - now;
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
	for (int i = 0; i<int(ans0.size()); ++i)
		EXPECT_EQ(ans1[i], i);
}
