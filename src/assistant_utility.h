﻿#pragma once
#ifndef ASSISTANT_UTILITY_HEAD
#define ASSISTANT_UTILITY_HEAD

#include<type_traits>
#include<concepts>
#include<vector>
#include<random>
#include<chrono>
#include<span>
#include<spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include<string_view>
#include<fmt/format.h>
#include<filesystem>
#include<nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include<fstream>
#include<iostream>
using namespace std;

inline nlohmann::json GetSetting(const string& settingFile);
inline shared_ptr<spdlog::logger> GetLogger(const string& name,const bool force_isolate=false){
	//@Todo: read log file root from json
	static auto logFileRoot = filesystem::absolute("./tmp/logs").string();
	//set all output into one file for debug
	if (!force_isolate&& name!= "integrated") return GetLogger("integrated");
	auto ret = spdlog::get(name);
	//if missing,create
	if (ret == nullptr) {
		{
			auto logfilename = fmt::format("{}/{}.log", logFileRoot, name);
			ret = spdlog::synchronous_factory::create<spdlog::sinks::basic_file_sink_st>
				(name, logfilename, false);
			//if create failed, return default
			if (!ret) {
				spdlog::warn("create log file[{}]failed {}:{}", logfilename, __FILE__, __LINE__);
				ret = spdlog::default_logger();
			}
		}
	}
	return ret;

}
template<typename T>
inline void LogExpectOrWarn(const string logName, T&& t,T expect) {
	if (t != expect) {
		GetLogger(logName)->warn("expect {} but get {}.{}:{}", expect, t, __FILE__, __LINE__);
	}
}

inline nlohmann::json GetSetting(const string& settingFile) {
	static nlohmann::json default_setting = R"({
	"setting_name":"default",
	"bool":{
		"true_value":true,
		"false_value":false
	},
	"string": "str_string"
})"_json;
	ifstream in(settingFile);
	nlohmann::json setting;
	if (in.good())
		in >> setting;
	else
	{
		GetLogger("json setting")->error("read setting file failed.{}[{}:{}]", settingFile, __FILE__, __LINE__);
		setting = default_setting;
	};
	return setting;

};
inline string ReadFile(const string& path) {
	fstream in(path);
	if (!in.good()) {
		GetLogger("default")->error("readfile[{}] failed.{}:{}",path, __FILE__, __LINE__);
		return "";
	}
		in.seekg(0, ios_base::end);
	size_t length = in.tellg();
	in.seekg(ios_base::beg);
	string ret;
	//the last char on file[length-1] is '\0'
	ret.resize(length);
	in.read(const_cast<char*>(ret.c_str()), length);
	//if (ret.back() == '\0')ret.pop_back();
	return ret;
}
inline bool WriteFile(const string& path, const string& content,const bool create_parent_dir_if_missing=true) {
	auto objDir = path;
	ofstream out(objDir);
	if (!out.good()) {
		auto parentDir = filesystem::path(objDir).parent_path();
		//maybe missing pg directory
		if (create_parent_dir_if_missing && !filesystem::is_directory(parentDir.c_str()))
		{
			GetLogger("default")->warn("write file[{}] failed because parent dir missed,now creating.{}:{}"
				,path, __FILE__, __LINE__);
			filesystem::create_directory(parentDir);
			out.open(objDir);
		}
		else {
			GetLogger("default")->warn("open file[{}] failed .{}:{}", path, __FILE__, __LINE__);
			return false;
		}
	}
	out << content << flush;
	out.close();
	return out.good();
}
inline auto Error_Exit() {
	spdlog::default_logger()->error("get error exit,check log for more info");
	exit(0);
}
inline std::string getTimeStr(std::string_view fmt) {
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char s[40] = {};
	std::strftime(&s[0], 40, fmt.data(), std::localtime(&now));
	return s;
}
inline std::string getTimeStr() {
	return getTimeStr("%Y-%m-%d %H:%M:%S");
}
//distribute task
template<typename SpreadNode, typename ChildTask, typename CollectRets, typename ...RestParams>
	requires invocable<ChildTask, SpreadNode, RestParams...>
&& invocable< CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
inline invoke_result_t<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
 SpreadCall(ChildTask ct, CollectRets cr, vector<SpreadNode> vs, RestParams&&... restParams) {
	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode, RestParams...>;
	vector<ChildTaskCallResultType> retparam;
	for (auto& node : vs)
		retparam.push_back(ct(node, forward<RestParams>(restParams)...));
	auto ret = cr(retparam);
	return ret; 
}

template<typename SpreadNode, typename ChildTask, typename ...RestParams>
	requires invocable<ChildTask, SpreadNode, RestParams...>
inline vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>
SpreadCall(ChildTask ct, vector<SpreadNode> vs, RestParams&&... restParams) {
	return SpreadCall(ct,
		[](vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>> rets) {return rets; },
		vs, forward<RestParams>(restParams)...);
}

template<typename T>
inline auto randomValue(T* des) {
	using DT = decay_t<T>;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	if constexpr (std::is_arithmetic_v<T>)
	{
		for (int i = 0; i<int(sizeof(DT)); ++i) {
			auto rando_result = (rando() % 255) + 1;
			*(reinterpret_cast<unsigned char*>(des)) = rando_result;
		}
	}
	else if constexpr (is_same_v<decay_t<T>, std::string>) {
		constexpr int len = 1000;
		static char buffer[len];
		static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
		const int strlen = (rando() % sizeof(buffer));
		for (int i = 0; i < strlen; ++i)
			buffer[i] = (rando() % ('z' - '0' + 1)) + '0';
		buffer[strlen] = '0';
		new(des) string(buffer);
	}
	else if constexpr (requires(T t) { T::randomValue(&t); }) {
		T::randomValue(des);
	}
	else if constexpr (requires(T t) { t.randomValue(); }) {
		des->randomValue();
	}
	else {
		//directly write down static_assert(false) would result in compile bug(or not?)
		//use is_same_v<T,T> to postphone analyse until specialization
		static_assert(!is_same_v<T, T>);
	}
}
//this interface seems to be irregular
template<typename T>
	requires is_arithmetic_v<T>
inline auto randomValue() {
	using DT = decay_t<T>;
	DT ret;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i<int(sizeof(DT)); ++i)
		*(reinterpret_cast<unsigned char*>(&ret)) = (rando() % 255) + 1;
	return ret;
}
template<typename T>
inline auto randomValue(T* beg, int len) {
	for (int i = 0; i < len; ++i)
		randomValue(beg + i);
}

//Serialize and Unserialize 
//buffer is obligate to control data block
class buffer {
public:
	static auto static_logger() {
		static auto _logger = spdlog::synchronous_factory::create<spdlog::sinks::basic_file_sink_st>
			("buffer", "logs/buffer.log", false);
		return _logger;
	}
	using UnitType = uint8_t;
	// using Container = vector<UnitType>;
	// data length
	int length = 0;
	// offset for read
	int offset = 0;
	//buffer length
	int buffer_length = 0;
	//data
	UnitType* data = nullptr;
	//
	static constexpr double incre_factor = 1.5;
	bool _expand_prepare(int addtionalLength) {
		if (addtionalLength + length > buffer_length)
			try {
			auto newBufferLength = (addtionalLength + length) * incre_factor;
			auto new_data = (UnitType*)malloc(sizeof(UnitType)*newBufferLength);// UnitType[newBufferLength];
			memcpy(new_data, data, length);
			buffer_length = newBufferLength;
			free(data);
			data = new_data;
		}
		catch (...) {
			//buffer::static_logger()->warn("{}:{},{} allocate buffer failed", getTimeStr(), __FILE__, __LINE__);
			return false;
		}

		return true;
	}
	~buffer() {
		free(data);
	}
};


//different from WriteArray,this requre T is arithmetic type
template<typename T >requires is_arithmetic_v<T>
inline void WriteSequence(buffer& buf, T* t, int len) {
	buf._expand_prepare(sizeof(T) * len);
	memcpy(buf.data + buf.length, t, sizeof(T) * len);
	buf.length += sizeof(T) * len;
}
//call for atithmetic,string and class with static or member Write(static first)
template<typename T = int>
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
	else if constexpr (requires(T t) { T::Write(declval< buffer&>(), &t); })
	{
		T::Write(buf, t);
	}
	else if constexpr (requires(T t) { t.Write(declval< buffer&>()); })
	{
		t->Write(buf);
	}
	else {
		static_assert(!is_same_v<T, T>);
	}

}

template<typename T>
inline void WriteArray(buffer& buf, T* t, int len)
{
	for (int i = 0; i < len; ++i)
		Write<T>(buf, t + i);
}
//return true if success
template<typename T>requires is_arithmetic_v<T>
inline bool ReadSequence(buffer& buf, T* t, int len) {
	constexpr auto TLENGTH = sizeof(T);
	if (buf.offset + TLENGTH * len > buf.length)
		return false;
	if constexpr (is_same_v<T, char>)
		memcpy(t, buf.data + buf.offset, TLENGTH * len);
	else
	{
		auto cast_t = (void*)(t);
		memcpy(cast_t, buf.data + buf.offset, TLENGTH * len);
	}

	buf.offset += TLENGTH * len;
	return true;
}
//return true if success
//call for arithmetic,string,and class with static or member Read(static first)
template<typename T>
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
	else if constexpr (requires(T t) { T::Read(declval<buffer&>(), &t); })
	{
		T::Read(buf, t);
	}
	else if constexpr (requires(T t) { t.Read(buf); }) {
		t->Read(buf);
	}
	else {
		static_assert(!is_same_v<T, T>);
	}
	return true;
}
//return true if success
template<typename T>
inline bool ReadArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		if (!Read(buf, t + i))return false;
	return true;
}
#endif //ASSISTANT_UTILITY_HEAD