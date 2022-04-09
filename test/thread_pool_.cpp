#include<test_head.h>
#include<thread_pool.h>
using namespace std;

auto conc = thread::hardware_concurrency();
TEST(thread_pool, basic) {
	GD::ThreadPool tp(conc);
	atomic_int tries = 10000, cnt = 0;
	for (int i = 0; i < tries; ++i)
		tp.enqueue([i, &cnt]() {cnt++; });
	tp.shutdown();
	//tp.shutdown();
	EXPECT_EQ(cnt, tries);
}