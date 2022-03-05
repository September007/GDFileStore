#include<gtest/gtest.h>
#include<template_utility.h>

TEST(template_utility,SpreadCall) {
	auto ct = [](int i) {
		cout << "handle child " << i << endl;
		return 1; 
	};
	auto cr = [](const vector<int>& vi) {
		cout << "handle collect" << endl;
		return vi.size(); 
	};
	constexpr int x = sizeof (int);
	cout << x << endl;
	auto ret = SpreadCall(ct, cr, vector<int>{1, 2, 3, 4, 5,6});
	auto ret1 = SpreadCall<int,decltype(ct),decltype(cr)>(ct,cr, vector<int>{ 1,2,3,4,5 });
	SpreadCall<int, decltype(ct), decltype(cr)>(ct, cr, vector<int>{});
}