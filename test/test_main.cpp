#include<test_head.h>

#include<gtest/gtest.h>
#include<iostream>
#include<deps/mem.h>
int main() {
	auto cv = (char*)GD_malloc(10, "test",__FILE__);
	constexpr auto x = is_constructible_v<PointerIntType, void*>;
	std::unordered_map<PointerIntType, HeapObjectInfo> uh;
	LOG_EXPECT_TRUE("integrate",false);
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