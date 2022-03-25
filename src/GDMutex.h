#pragma once
#ifndef GD_MUTEX_HEAD
#define GD_MUTEX_HEAD
#include<mutex>
#include<map>

enum class mutex_enum {
	//for x
	header_lock,
};
//@Todo : one day, all the places where GetMutex used ,may change logic to as mutex bind as object's member
// but this is freer
template<typename ...ArgsType>
std::mutex& GetMutex(ArgsType...args) {
	using keyT = std::tuple<ArgsType...>;
	auto key = make_tuple<ArgsType...>(std::forward<ArgsType>(args)...);
	static std::map < keyT, mutex> m;
	static mutex m_mutex;
	lock_guard lg(m_mutex);
	return m[key];
}


#endif //GD_MUTEX_HEAD