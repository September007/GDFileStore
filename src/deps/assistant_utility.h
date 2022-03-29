﻿/* this file implement
* log,formatted time,IO,serialize,
* @Contract
*	1.	if specified class is plained(has no pointer memeber like raw pointer or shared_ptr),
*		you only need achieve the member function GetES() to point out member address,
*		but if not, you should achieve three more function to do some spcified work
*		(you still could use GetES() to do with other non-pointer member),check class::Slice see detail;
*	2.	randomValue,Write,Read do not expect type tuple(lazy)
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
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <type_traits>
#include <vector>
#include<memory>
using namespace std;
template<template<typename...args>typename templateType, typename T>
constexpr bool is_from_template = false;
template<template<typename...args>typename templateType, typename...args>
constexpr bool is_from_template<templateType, templateType<args...>> = true;;
template<typename T>
constexpr bool is_tuple = is_from_template<tuple, T>;

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

// suppose pointer t don't need call destruction
template<typename T>
void randomValue(T* t, int len = 1,bool call_des=false) {
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	using DT = decay_t<T>;
	if constexpr (is_same_v<decay_t<T>, uint8_t>) {
		for (int i = 0; i < len; ++i)
			*(uint8_t*)(t+i) = (rando() % (UINT8_MAX + 1));
	}
	else if constexpr (is_integral_v<T> || is_enum_v<T>) {
		randomValue<uint8_t>(const_cast<uint8_t*>( reinterpret_cast<const uint8_t*>(t)), len * sizeof(T) / sizeof(uint8_t));
	}
	else if constexpr (is_floating_point_v<T>) {
		static  uniform_real_distribution<DT> urd(numeric_limits<DT>::min(), numeric_limits<DT>::max());
		for (int i = 0; i < len;++i)
			*const_cast<DT*>(t+i) = urd(rando);
	}
	else if constexpr (is_same_v<string, decay_t<T>>) {
		for (int i = 0; i < len; ++i) {
			int slen = rando() % 100 + 1;
			new((string*)(t)+i)string(slen, '\0');
			randomValue<uint8_t>(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(t[i].data())), slen);
		}
	}
	else if constexpr (requires(T _t) { _t.GetES(); }) {
		for (int i = 0; i < len; ++i) {
			auto es = t[i].GetES();
			randomValue(&es);
			//for more random
			if constexpr (requires(decay_t<T>_t) { T::RandomValue(&_t); })
				T::RandomValue((T * )(t)+i);
		}
	}
	else if constexpr (is_same_v<T, tuple<>>) {}
	else if constexpr (is_tuple<T>) {
		//expect tuple only come from class.GetES()
		LOG_EXPECT_EQ("randomValue", len, 1);
		auto rest = t->_Get_rest();
		randomValue(t->_Myfirst._Val);
		randomValue(&rest);
	}
	else if constexpr (is_from_template<vector, T>) {
		for (int i = 0; i < len; ++i) {
			int sz = rando() % 10 + 1;
			new(t+i)T(sz);
			randomValue<typename T::value_type>(&(*t)[0], sz);
		}
	}
	else if constexpr (is_same_v<T, T>) {
		static_assert(!is_same_v<T, T>, "can't found suited imple,plz make it");
	}
}
template<typename T>
auto randomValue() {
	uint8_t buf[sizeof(T)];
	randomValue<T>(reinterpret_cast<T*>(buf), 1);
	return move(*reinterpret_cast<T*>(buf));
}

// Serialize and Unserialize
// buffer is obligate to control data block
struct buffer {
public:
	string data;
	int offset = 0;
	buffer(const string data = "",int offset=0) :data(data) ,offset(offset) {}
	bool operator==(const buffer& buf) { return this->data == buf.data && this->offset == buf.offset; }
	void append(int len, const void* src) {
		data.append(len, '\0');
		memcpy(data.data() + data.size() - len, src, len);
	}
	void drawback(int len, void* des) {
		memcpy(des, data.data() + offset, len);
		offset += len;
	}
	string universal_str() { return data; }
	auto GetES() { return make_tuple(&data, &offset); }
};

class Slice {
public:
	shared_ptr<buffer> data;
	int start;
	int end;
	Slice(shared_ptr<buffer> data=nullptr, int start=0, int end=0)
		: data(data)
		, start(start)
		, end(end) {
	}
	explicit Slice(shared_ptr<buffer> data)
		: data(data)
		, start(0)
		, end(data->data.length()) {
	}
	explicit Slice(const string& str);
	Slice(const Slice&) = default;
	// value cmp
	bool operator==(const Slice& sl) const {
		return this->start == sl.start && this->end == sl.end &&
			((this->data == sl.data && this->data == nullptr) ||
				(this->data != nullptr && sl.data != nullptr && (*this->data == *sl.data)));
	}
	int GetSize() const { return end - start; }
	Slice SubSlice(int _start, int _size) {
		if (_start + _size < GetSize())
			return Slice(data, this->start + _start, this->start + _start + _size);
		else
			return Slice(data, this->start + _start, this->end);
	}
	// support for serilize and rando
	static void Read(buffer& buf, Slice* sli);
	static void Write(buffer& buf, Slice* s);
	static void RandomValue(Slice* sli) { new(&sli->data)shared_ptr<buffer>(make_shared<buffer>()); }
	auto GetES() { return make_tuple(&start, &end); }
};

template< typename T>
void Write(buffer& buf, T* t);
template<typename T>
void Read(buffer& buf, T* t);
template<typename T>
decay_t<T> Read(buffer& buf);
template<typename T>
void WriteArray(buffer& buf, T* t, int len);
template<typename T>
void ReadArray(buffer& buf, T* t, int len);

template< typename T>
void Write(buffer& buf, T* t) {
	constexpr int TLEN = sizeof(T);
	if constexpr (is_arithmetic_v<T> || is_enum_v<T>) {
		buf.append(TLEN, (void*)t);
	}
	else if constexpr (is_same_v<string, decay_t<T>>) {
		int len = t->length();
		buf.append(sizeof(int), &len);
		buf.append(len * sizeof(char), t->data());
	}
	else if constexpr (requires(T _t) { _t.GetES(); }) {
		auto es = t->GetES();
		Write(buf, &es);
		// more action ,may for shared_ptr<T> balabalah
		if constexpr (requires(T t) { T::Write(declval<buffer&>(), &t); T::Read(declval<buffer&>(), &t); }) {
			T::Write(buf, t);
		}
	}
	else if constexpr (is_same_v<decay_t<T>, tuple<>>) return;
	else if constexpr (is_tuple<decay_t<T>>) {
		Write(buf, t->_Myfirst._Val);
		auto rest = t->_Get_rest();
		Write(buf, &rest);
	}
	else if constexpr (is_from_template<vector, T>) {
		int sz = t->size();
		buf.append(sizeof(sz), &sz);
		WriteArray(buf, &(*t)[0], sz);
	}
	else if constexpr (is_same_v<T,T>) {
		//static_assert(!is_same_v<T, T>,"can't found suited imple of write");
		LOG_ERROR("integrated", "can't found suited imple of write");
	}
}

template<typename T>
decay_t<T> Read(buffer& buf) {
	constexpr int TLEN = sizeof(T);
	using DT = decay_t<T>;
	char cache[TLEN];
	Read<DT>(buf, reinterpret_cast<DT*>(cache));
	return move(*(reinterpret_cast<DT*>(cache)));
}

template<typename T>
void Read(buffer& buf, T* t) {
	//if (call_Des)
		//t->~T();
	constexpr int TLEN = sizeof(T);
	using DT = decay_t<T>;
	if constexpr (is_arithmetic_v<T> || is_enum_v<T>) {
		buf.drawback(TLEN, t);
	}
	else if constexpr (is_same_v<DT, string>) {
		auto len = Read<int>(buf);
		new((string*)(t))string(size_t(len), '\0');
		buf.drawback(len, const_cast<char*>(t->data()));
	}
	else if constexpr (requires(T _t) { _t.GetES(); }) {
		auto es = t->GetES();
		Read(buf, &es);
		// more action ,may for shared_ptr<T> balabalah
		if constexpr (requires(T _t) { T::Write(declval<buffer&>(), &_t); T::Read(declval<buffer&>(), &_t); }) {
			T::Read(buf, t);
		}
	}
	else if constexpr (is_same_v<tuple<>, DT>) {}
	else if constexpr (is_tuple<T>) {
		auto rest = t->_Get_rest();
		Read(buf, t->_Myfirst._Val);
		Read(buf, &rest);
	}
	else if constexpr (is_from_template<vector, T>) {
		int sz;
		buf.drawback(sizeof(sz), &sz);
		new(t)T(sz);
		ReadArray(buf, &(*t)[0], sz);
	}
	else if constexpr (is_same_v<T, T>) {
		static_assert(!is_same_v<T, T>, "can't found right imple");
	}
}
template<typename T>
void WriteArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		Write(buf, t + i);
}
template<typename T>
void ReadArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		Read(buf, t + i);
}
template<typename T>
vector<decay_t<T>>  ReadArray(buffer& buf, int len) {
	static_assert(is_constructible_v<decay_t<T>>, "ReadArray will call default contruction,but there is not one"); 
	vector<decay_t<T>> ret(len);
	for (int i = 0; i < len; ++i)
		Read(buf, (&ret[0] + i));
	return ret;
}
template<typename T> requires(is_arithmetic_v<T> || is_enum_v<T>)
void WriteSequence(buffer& buf, T* t, int len) {
	buf.append(sizeof(T) * len, t);
}
template<typename T> requires(is_arithmetic_v<T> || is_enum_v<T>)
void	ReadSequence(buffer& buf, T* t, int len) {
	buf.drawback(sizeof(T) * len, t);
}
#endif  // ASSISTANT_UTILITY_HEAD