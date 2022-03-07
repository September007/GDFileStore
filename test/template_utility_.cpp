#include<gtest/gtest.h>
#include<assistant_utility.h>
//#include<data.h>
TEST(assistant_utility, SpreadCall) {
	vector<int> params = { 1,2,3,4,5,5,6,6,6,3,6,3,7,7,7,77,7,3,7 };
	auto ret0 = 0, ret1 = 0, ret2 = 0;
	auto ct = [&](int i, int& ret) {
		ret += i;
		return i;
	};
	auto cr = [&](const vector<int>& vi) {
		return vi;
	};
	//call spreadcall
	auto retParams = SpreadCall(ct, cr, params, ret0);
	//call each one  
	for (auto p : params)ct(p, ret1);
	//call another spreadcall
	auto retParams1 = SpreadCall<int, decltype(ct)>(ct, params, ret2);
	EXPECT_EQ(ret0, ret1);
	EXPECT_EQ(params, retParams);
	EXPECT_EQ(params, retParams1);
};
TEST(assistant_utility, randomValue) {
	constexpr auto tries = 1000;
	for (int i = 0; i < tries; ++i) {
		randomValue<bool>();
		randomValue<const bool>();
		randomValue<float>();
		randomValue<const float>();
		randomValue<float>();
		randomValue<const float>();
		randomValue<double>();
		randomValue<const double>();
		randomValue<int>();
		randomValue<const int>();
		string s;
		randomValue(&s);
	}
};
TEST(assistant_utility, serialize) {
	//buffer bf;
	
}