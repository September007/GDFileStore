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
		if (rd != vec[i])
			ret++;
	}
	//test WriteArray
	T* datas = new T[tries], * read_datas = new T[tries];
	WriteArray(buf, datas, tries);
	ReadArray(buf, read_datas, tries);
	for (int i = 0; i < tries; ++i) {
		EXPECT_EQ(datas[i], read_datas[i]);
		if(datas[i] != read_datas[i])
			ret++;
	}
	//test WriteSequence
	if constexpr (is_arithmetic_v<T>) {
		WriteSequence(buf, datas, tries);
		memset(read_datas, 0, tries * sizeof(T));
		ReadSequence(buf, read_datas, tries);
		for (int i = 0; i < tries; ++i) {
			auto x = datas[i] == read_datas[i];
			EXPECT_TRUE(x);
			if (!x)
				ret++;
		}
	}
	delete[]datas, delete[]read_datas;
	return ret;
}
class _clsWithStaticInterface {
public:
	int i;
	float f;
	string s;
	bool operator==(const _clsWithStaticInterface&) const= default;
	static void Write(buffer& buf, _clsWithStaticInterface *cls) {
		::Write(buf, &cls->i);
		::Write(buf, &cls->f);
		::Write(buf, &cls->s);
	}
	static bool Read(buffer& buf, _clsWithStaticInterface *cls) {
		return ::Read(buf, &cls->i) &&
			   ::Read(buf, &cls->f) &&
			   ::Read(buf, &cls->s);
	}
	void randomValue() {
		::randomValue(&this->i);
		::randomValue(&this->f);
		::randomValue(&this->s);
	}
};
class _clsWithMemberInterface {
public:
	int i;
	float f;
	string s;
	bool operator==(const _clsWithMemberInterface&) const = default;
	void Write(buffer& buf) {
		::Write(buf, &this->i);
		::Write(buf, &this->f);
		::Write(buf, &this->s);
	}
	bool Read(buffer& buf) {
		return ::Read(buf, &this->i) &&
			   ::Read(buf, &this->f) &&
			   ::Read(buf, &this->s);
	}
	void randomValue() {
		::randomValue(&this->i);
		::randomValue(&this->f);
		::randomValue(&this->s);
	}
};
TEST(assistant_utility, serialize) {
	buffer buf;
	//string s;
	//randomValue(&s);
	//Write(buf, &s);
	//string read_s;
	//Read(buf, &read_s);
	//EXPECT_EQ(s,read_s);
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
	EXPECT_EQ(TestOneKindOfType<_clsWithMemberInterface>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<_clsWithStaticInterface>(buf), 0);
	_clsWithStaticInterface _cls, r_cls;
	_cls.i = 100, _cls.f = 1000, _cls.s = "dasda";
	Write(buf, &_cls);
	Read(buf, &r_cls);
	EXPECT_EQ(_cls, r_cls);
	_clsWithMemberInterface _mcls, r_mcls;
	_mcls.i = 100, _mcls.f = 1000, _mcls.s = "dasda";
	Write(buf, &_mcls);
	Read(buf, &r_mcls);
	EXPECT_EQ(_mcls, r_mcls);

}

TEST(file, read) {
	string	path("./testout.txt");
	ofstream out(path);
	char str[10] = { 'a','b','c','\0','e','f','h','8','9','\0'};
	for (auto c : str)
		out << c;
	out.close();
	auto read = ReadFile(path);
	EXPECT_EQ(read.size(), 10);
	for (int i = 0; i < 10; ++i)
		EXPECT_EQ(read[i], str[i]);
}