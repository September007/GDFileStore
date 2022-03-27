/* this file implement 
* log,formatted time,IO,serialize,
*/
#pragma once
#ifndef ASSISTANT_UTILITY_HEAD
#define ASSISTANT_UTILITY_HEAD

#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>
#include<memory>
using namespace std;

shared_ptr<spdlog::logger> GetLogger(const string& name,
	const bool force_isolate = false, const string& fileClass = "integrated");
template <typename T>
inline void LogExpectOrWarn(const string logName, T&& t, T expect) {
	if (t != expect) {
		GetLogger(logName)->warn("expect {} but get {}.{}:{}", expect, t, __FILE__,
			__LINE__);
	}
}
#pragma region assert
#define LOG_ASSERT_TRUE(logName,condi,msg) do { if((condi)!=true) GetLogger(logName)->error(msg" expect true but get false. [{}]at {}:{}"\
        ,#condi,__FILE__,__LINE__);}while(0)
#define LOG_EXPECT_TRUE(logName,condi) do { if((condi)!=true) GetLogger(logName)->error("expect true but get false. [{}]at {}:{}"\
        ,#condi,__FILE__,__LINE__);}while(0)
#define LOG_EXPECT_EQ(logName,l,r) do { if((l)!=(r)) GetLogger(logName)->error("expect equal but not. [{}:{}]!=[{}:{}]at {}:{}"\
        ,#l,l,#r,r,__FILE__,__LINE__);}while(0)

#define LOG_INFO(logName,msg) do { GetLogger(logName)->info("{} at {}:{}",msg,__FILE__,__LINE__);}while(0)

#define LOG_WARN(logName,msg) do { GetLogger(logName)->warn("msg at {}:{}",msg,__FILE__,__LINE__);}while(0)

#define LOG_ERROR(logName,msg) do { GetLogger(logName)->error("{} at {}:{}",msg,__FILE__,__LINE__);}while(0)

#define randomDefine x
#pragma endregion
//Todo: json setting file
nlohmann::json GetSetting(const string& settingFile);
string ReadFile(const string& path);
bool WriteFile(const string& path,	const string& content,	const bool create_parent_dir_if_missing = true);
void Error_Exit();
std::string getTimeStr(std::string_view fmt);
inline std::string getTimeStr() {
	return getTimeStr("%Y-%m-%d %H:%M:%S");
}
// distribute task
template <typename SpreadNode,
	typename ChildTask,
	typename CollectRets,
	typename... RestParams>
	requires invocable<ChildTask, SpreadNode, RestParams...>&&
invocable<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
inline invoke_result_t<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
SpreadCall(ChildTask ct, CollectRets cr, vector<SpreadNode> vs, RestParams&&... restParams) {
	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode, RestParams...>;
	vector<ChildTaskCallResultType> retparam;
	for (auto& node : vs)
		retparam.push_back(ct(node, forward<RestParams>(restParams)...));
	auto ret = cr(retparam);
	return ret;
}

template <typename SpreadNode, typename ChildTask, typename... RestParams>
	requires invocable<ChildTask, SpreadNode, RestParams...>
inline vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>
SpreadCall(ChildTask ct, vector<SpreadNode> vs, RestParams&&... restParams) {
	return SpreadCall(
		ct,
		[](vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>> rets) {
			return rets;
		},
		vs, forward<RestParams>(restParams)...);
}

template <typename T>
inline auto randomValue(T* des) {
	using DT = decay_t<T>;
	static mt19937_64 rando(
		chrono::system_clock::now().time_since_epoch().count());
	if constexpr (std::is_arithmetic_v<T>) {
		for (int i = 0; i < int(sizeof(DT)); ++i) {
			auto rando_result = (rando() % 255) + 1;
			*(reinterpret_cast<unsigned char*>(des)) = rando_result;
		}
	}
	else if constexpr (is_same_v<decay_t<T>, std::string>) {
		constexpr int len = 1000;
		static char buffer[len];
		static mt19937_64 rando(
			chrono::system_clock::now().time_since_epoch().count());
		const int strlen = (rando() % sizeof(buffer));
		for (int i = 0; i < strlen; ++i)
			buffer[i] = (rando() % ('z' - '0' + 1)) + '0';
		buffer[strlen] = '0';
		new (des) string(buffer);
	}
	else if constexpr (requires(T t) { T::randomValue(&t); }) {
		T::randomValue(des);
	}
	else if constexpr (requires(T t) { t.randomValue(); }) {
		des->randomValue();
	}
	else {
		// directly write down static_assert(false) would result in compile bug(or
		// not?) use is_same_v<T,T> to postphone analyse until specialization
		static_assert(!is_same_v<T, T>);
	}
}
// this interface seems to be irregular
template <typename T>
	requires is_arithmetic_v<T>
inline auto randomValue() {
	using DT = decay_t<T>;
	DT ret;
	static mt19937_64 rando(
		chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i < int(sizeof(DT)); ++i)
		*(reinterpret_cast<unsigned char*>(&ret)) = (rando() % 255) + 1;
	return ret;
}
template <typename T>
inline auto randomValue(T* beg, int len) {
	for (int i = 0; i < len; ++i)
		randomValue(beg + i);
}

// Serialize and Unserialize
// buffer is obligate to control data block
class buffer {
public: 
	buffer() {}
	buffer(const string& s);
	using UnitType = uint8_t;
	// using Container = vector<UnitType>;
	// data length
	int length = 0;
	// offset for read
	int offset = 0;
	// buffer length
	int buffer_length = 0;
	// data
	UnitType* data = nullptr;
	//
	static constexpr double incre_factor = 1.5;
	bool _expand_prepare(int addtionalLength);
	//only when sz > buffer_length, trigger a buffer expand
	void _Reserve(int sz) {
		if (sz > buffer_length) {
			_expand_prepare(sz - buffer_length);
		}
	}
	string universal_str() {
		string ret(this->length,'\0');
		memcpy(&ret[0], this->data, this->length);
		return ret;
	}
	~buffer() { free(data); }
	//serialize 
	//@Contract_1 read and write as a char *
	static bool Read(buffer& buf, buffer* rbuf);
	static void Write(buffer& buf, buffer* wbuf);
};


// need gthis delaration
inline shared_ptr<buffer> BufferFrom(const char* p, int sz);
inline shared_ptr<buffer>  BufferFrom(const string& str);

class Slice {
public:
	shared_ptr<buffer> data;
	int start;
	int end;
	Slice(shared_ptr<buffer> data, int start, int end)
		: data(data)
		, start(start)
		, end(end) {
	}
	explicit Slice(shared_ptr<buffer> data)
		: data(data)
		, start(0)
		, end(data->length) {
	}
	explicit Slice(const string& str);
	Slice(const Slice&) = default;
	bool operator==(const Slice&) const = default;
	int GetSize() const { return end - start; }
	Slice SubSlice(int _start, int _size) {
		if (_start + _size < GetSize())
			return Slice(data, this->start + _start, this->start + _start + _size);
		else
			return Slice(data, this->start + _start, this->end);
	}
	// support for serilize
	static bool Read(buffer& buf, Slice* sli);
	static void Write(buffer& buf, Slice* s);
};
// different from WriteArray,this requre T is arithmetic type
template <typename T>
	requires is_arithmetic_v<T>
inline void WriteSequence(buffer& buf, T* t, int len) {
	buf._expand_prepare(sizeof(T) * len);
	memcpy(buf.data + buf.length, t, sizeof(T) * len);
	buf.length += sizeof(T) * len;
}
// call for atithmetic,string and class with static or member Write(static
// first)
template <typename T = int>
inline void Write(buffer& buf, T* t) {
	constexpr auto TLENGTH = sizeof(T);
	if constexpr (is_arithmetic_v<T>) {
		buf._expand_prepare(TLENGTH);
		memcpy(buf.data + buf.length, reinterpret_cast<void*>(t), TLENGTH);
		buf.length += TLENGTH;
	}
	else if constexpr (is_same_v<decay_t<T>, std::string>) {
		int sz = t->length();
		Write(buf, &sz);
		WriteSequence(buf, t->c_str(), t->length() * sizeof(char));
	}
	else if constexpr (requires(T _t) { T::Write(declval<buffer&>(), &_t); }) {
		T::Write(buf, t);
	}
	else if constexpr (requires(T t) { t.Write(declval<buffer&>()); }) {
		t->Write(buf);
	}
	else if constexpr (is_same_v<T, Slice>) {
		int length = t->GetSize();
		Write(buf, &length);
		buf._expand_prepare(length);
		memcpy(buf.data + buf.length, t->data.data[t->start], length);
		buf.length += length;
	}
	else {
		static_assert(!is_same_v<T, T>);
	}
}

template <typename T>
inline void WriteArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		Write<T>(buf, t + i);
}
// return true if success
template <typename T>
	requires is_arithmetic_v<T>
inline bool ReadSequence(buffer& buf, T* t, int len) {
	constexpr auto TLENGTH = sizeof(T);
	if (buf.offset + TLENGTH * len > buf.length)
		return false;
	if constexpr (is_same_v<T, char>)
		memcpy(t, buf.data + buf.offset, TLENGTH * len);
	else {
		auto cast_t = (void*)(t);
		memcpy(cast_t, buf.data + buf.offset, TLENGTH * len);
	}

	buf.offset += TLENGTH * len;
	return true;
}
class InfoForOSD;
// return true if success
// call for arithmetic,string,and class with static or member Read(static first)
template <typename T>
inline bool Read(buffer& buf, T* t) {
	if constexpr (is_arithmetic_v<T>) {
		ReadSequence(buf, t, 1);
	}
	else if constexpr (is_same_v<decay_t<T>, std::string>) {
		int sz = 0;
		ReadSequence(buf, &sz, 1);
		string str(sz, '0');
		ReadSequence(buf, str.c_str(), sz);
		const_cast<string*>(t)->swap(str);
	}
	else if constexpr (requires(T t) { T::Read(declval<buffer&>(), &t); }) {
		T::Read(buf, t);
	}
	else if constexpr (requires(T t) { t.Read(buf); }) {
		t->Read(buf);
	}
	else if constexpr (is_same_v<T, Slice>) {
		int length = 0;
		Read(buf, &length);
		shared_ptr<buffer> slice_buffer;
		WriteSequence(*slice_buffer.get(), buf.data, length);
		buf.offset += length;
		*t = Slice(slice_buffer, 0, length);
	}
	else {
		static_assert(!is_same_v<T, T>,"cant found relevant read implementation of Type T");
	}
	return true;
}
// return true if success
template <typename T>
inline bool ReadArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		if (!Read(buf, t + i))
			return false;
	return true;
}
// special use of read
template<typename T>
T ReadConstruct(buffer& buf) {
	constexpr auto TLEN = sizeof(T);
	unsigned char b[TLEN];
	::Read<T>(buf, reinterpret_cast<T*>(b));
	return move(*reinterpret_cast<T*>(b));
}
template<typename T>
vector<T> ReadArrayConstruct(buffer& buf,int len) {
	constexpr auto TLEN = sizeof(T);
	unsigned char *b = new unsigned char[TLEN + len];
	auto tb = reinterpret_cast<T*>(b);
	for (int i = 0; i < len; ++i)
		::Read<T>(buf, tb + i);
	vector<T> ret(tb, tb + len);
	return ret;
}
//construct buffer


shared_ptr<buffer> BufferFrom(const char* p, int sz);
shared_ptr<buffer>  BufferFrom(const string& str);

#endif  // ASSISTANT_UTILITY_HEAD