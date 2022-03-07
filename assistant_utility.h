#pragma once
#ifndef TEMPLATE_UTILITY_HEAD
#define TEMPLATE_UTILITY_HEAD

#include<type_traits>
#include<concepts>
#include<vector>
#include<random>
#include<chrono>
#include<span>
#include<spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include<string_view>
using namespace std;


inline std::string getTimeStr(std::string_view fmt) {
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::string s(30, '\0');
	std::strftime(&s[0], s.size(), fmt.data(), std::localtime(&now));
	return s;
}
inline std::string getTimeStr() {
	return getTimeStr("%Y-%m-%d %H:%M:%S");
}
//distribute task
template<typename SpreadNode, typename ChildTask, typename CollectRets, typename ...RestParams>
	requires invocable<ChildTask, SpreadNode, RestParams...>
&& invocable< CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
invoke_result_t<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
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
vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>
SpreadCall(ChildTask ct, vector<SpreadNode> vs, RestParams&&... restParams) {
	return SpreadCall(ct,
		[](vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>> rets) {return rets; },
		vs, forward<RestParams>(restParams)...);
}

template<typename T>
	requires is_move_assignable_v<decay_t<T>>&& is_arithmetic_v<T>
auto randomValue(T* des) {
	using DT = decay_t<T>;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i<int(sizeof(DT)); ++i) {
		auto rando_result = (rando() % 255) + 1;
		*(reinterpret_cast<unsigned char*>(des)) = rando_result;
	}
}
//addtional support for string
inline void randomValue(string* str);
template<typename T>
	requires is_move_assignable_v<decay_t<T>>&& is_arithmetic_v<T>&& is_constructible_v<T, decay_t<T>>
auto randomValue() {
	using DT = decay_t<T>;
	DT ret;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i<int(sizeof(DT)); ++i)
		*(reinterpret_cast<unsigned char*>(&ret)) = (rando() % 255) + 1;
	return ret;
}
template<typename T>
	requires is_move_assignable_v<T>&& is_arithmetic_v<T> || is_same_v<decay_t<T>, string>
auto randomValue(T * beg, int len) {
	for (int i = 0; i < len; ++i)
		randomValue(beg + i);
}

inline void randomValue(string* str) {
	constexpr int len = 1000;
	static char buffer[len];
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	const int strlen = (rando() % sizeof(buffer));
	for (int i = 0; i < strlen; ++i)
		buffer[i] = (rando() % ('z' - '0' + 1)) + '0';
	buffer[strlen] = '0';
	new(str) string(buffer);
}

//Serialize and Unserialize , usr std::span
// using buffer_base = std::span<uint8_t,std::dynamic_extent>;


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
			auto new_data = new UnitType[newBufferLength];
			memcpy(new_data, data, length);
			buffer_length = newBufferLength;
			delete data;
			data = new_data;
		}
		catch (...) {
			//buffer::static_logger()->warn("{}:{},{} allocate buffer failed", getTimeStr(), __FILE__, __LINE__);
			return false;
		}

		return true;
	}
	~buffer() {
		delete data;
	}
};

//different from WriteArray,this requre T is pod type
template<typename T >requires is_arithmetic_v<T>
void WriteSequence(buffer& buf, T* t, int len) {
	buf._expand_prepare(sizeof(T) * len);
	memcpy(buf.data + buf.length, t, sizeof(T) * len);
	buf.length += sizeof(T) * len;
}

template<typename T = int>
void Write(buffer& buf, T* t) {
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

}
template<typename T = int>
void WriteArray(buffer& buf, T* t, int len);

template<typename T>
void WriteArray(buffer& buf, T* t, int len)
{
	for (int i = 0; i < len; ++i)
		Write<T>(buf, t + i);
}
template<typename T>requires is_arithmetic_v<T>
bool ReadSequence(buffer& buf, T* t, int len) {
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
template<typename T>
bool Read(buffer& buf, T* t) {
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
	return true;
}
template<typename T>
bool ReadArray(buffer& buf, T* t, int len) {
	for (int i = 0; i < len; ++i)
		if (!Read(buf, t))return false;
	return true;
}
#endif //TEMPLATE_UTILITY_HEAD