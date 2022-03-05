#include<gtest/gtest.h>
#include<iostream>
int main() {
	::testing::InitGoogleTest();
	auto ret=RUN_ALL_TESTS();
}