#include<test_head.h>

#include<gtest/gtest.h>
#include<iostream>
#include<deps/mem.h>
int main() {
#ifdef __SANITIZE_ADDRESS__
	printf("Address sanitizer enabled\n");
#else
	printf("Address sanitizer not enabled\n");
#endif
	::testing::InitGoogleTest();
	auto ret = RUN_ALL_TESTS();
#ifdef __linux__
	std::getchar();
#endif
	HeapRecorder::instance().Report();
	return 0;
}