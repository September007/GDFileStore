#include<gtest/gtest.h>
#include<assistant_utility.h>
//#include<data.h>
constexpr auto tries = 100;
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
template<typename T>
auto TestOneKindOfType(buffer& buf) {
	int ret = 0;
	vector<T> vec(tries);
	//if T is string, when generate and fill data,the original string would be directly erase instead of destory
	// there would not call deconstruction function
	randomValue(&vec[0], tries);
	//test Write
	for (int i = 0; i < tries; ++i) {
		Write(buf, &vec[i]);
		T rd;
		Read(buf, &rd);
		EXPECT_EQ(rd, vec[i]);
		ret += rd != vec[i];
	}
	//test WriteArray
	T* datas = new T[tries], * read_datas = new T[tries];
	WriteArray(buf, datas, tries);
	ReadArray(buf, read_datas, tries);
	for (int i = 0; i < tries; ++i) {
		EXPECT_EQ(datas[i], read_datas[i]);
		ret += datas[i] != read_datas[i];
	}
	//test WriteSequence
	if constexpr (is_arithmetic_v<T>) {
		WriteSequence(buf, datas, tries);
		memset(read_datas, 0, tries * sizeof(T));
		ReadSequence(buf, read_datas, tries);
		for (int i = 0; i < tries; ++i) {
			auto x = datas[i] == read_datas[i];
			EXPECT_TRUE(x);
			ret += !x;
		}
	}
	delete[]datas, delete[]read_datas;
	return ret;
}
TEST(assistant_utility, serialize) {
	buffer buf;
	string s;
	randomValue(&s);
	Write(buf, &s);
	string read_s;
	Read(buf, &read_s);
	EXPECT_EQ(s,read_s);
	EXPECT_EQ(TestOneKindOfType<char>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<uint8_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<uint16_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<uint32_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<uint64_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<int8_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<int16_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<int32_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<int64_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<string>(buf), 0);
	int asda;
}