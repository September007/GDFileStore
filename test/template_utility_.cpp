#include<gtest/gtest.h>
#include<assistant_utility.h>

TEST(template_utility,SpreadCall) {
	vector<int> params={1,2,3,4,5,5,6,6,6,3,6,3,7,7,7,77,7,3,7};
	auto ret0=0,ret1=0,ret2=0;
	auto ct = [&](int i,int& ret) {
		ret+=i;
		return i; 
	};
	auto cr = [&](const vector<int>& vi) {
		return vi; 
	};
	//call spreadcall
	auto retParams = SpreadCall(ct, cr, params,ret0);
	//call each one  
	for (auto p : params)ct(p,ret1);
	//call another spreadcall
	auto retParams1=SpreadCall<int,decltype(ct)>(ct, params,ret2);
	EXPECT_EQ(ret0,ret1);
	EXPECT_EQ(params,retParams);
	EXPECT_EQ(params,retParams1);
}