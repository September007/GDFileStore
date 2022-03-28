#include<test_head.h>
#include<gtest/gtest.h>
#include<assistant_utility.h>
#include<object.h>

//#include<data.h>
constexpr auto tries = 100;
#define head assistant_utility

template<typename T>
bool value_cmp(const T& l, const T& r) { 
	if constexpr (is_same_v<T, Slice>) {
		return l.start == r.start && l.end == r.end
			&& ((l.data == r.data && l.data == nullptr) || (l.data !=  nullptr && r.data != nullptr && *l.data == *r.data));
	}
	return l == r; 
}

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
		auto& vi = vec[i];
		T rd;
		Read(buf, &rd);
		auto x = value_cmp(rd, vi);
		EXPECT_TRUE(x);
		if (!x)
			ret++;
	}
	//test WriteArray
	T *datas = new T[tries], * read_datas = new T[tries];
	randomValue(datas, tries);
	WriteArray(buf, datas, tries);
	ReadArray(buf, read_datas, tries);
	for (int i = 0; i < tries; ++i) {
		auto& di = datas[i], & ri = read_datas[i];
		auto x = value_cmp(datas[i], read_datas[i]);
		EXPECT_TRUE(x);
		if(!x)
			ret++;
	}
	//test WriteSequence
	if constexpr (is_arithmetic_v<T>) {
		WriteSequence(buf, datas, tries);
		memset(read_datas, 0, tries * sizeof(T));
		ReadSequence(buf, read_datas, tries);
		for (int i = 0; i < tries; ++i) {
			auto x = value_cmp(datas[i], read_datas[i]);
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
	auto GetES() { return make_tuple(&i, &f, &s); }
};
class _clsWithMemberInterface {
public:
	int i;
	float f;
	string s;
	bool operator==(const _clsWithMemberInterface&) const = default;
	auto GetES() { return make_tuple(&i, &f, &s); }
};
TEST(assistant_utility, serialize) {
	buffer buf;
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
}
TEST(assistant_utility, serialize_object_class) {
	buffer buf;
	//class in object.h
	EXPECT_EQ(TestOneKindOfType<Slice>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<FileAnchor>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<FilePos>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<Object_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<HObject_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<gen_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<shard_id_t>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<GHObject_t>(buf), 0); constexpr int lghobj = sizeof(GHObject_t);
	EXPECT_EQ(TestOneKindOfType<OperationType>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<Operation>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<OperationState>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<PageGroup>(buf), 0);
	EXPECT_EQ(TestOneKindOfType<Snapid_t>(buf), 0);
}