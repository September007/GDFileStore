#pragma once
#ifndef TEMPLATE_UTILITY_HEAD
#define TEMPLATE_UTILITY_HEAD

#include<type_traits>
#include<concepts>
#include<vector>
#include<random>
#include<chrono>
using namespace std;

template<typename T1,typename T2>
auto max(T1 l, T2 r) {
	return l > r ? l : r;
}
//distribute task
template<typename SpreadNode, typename ChildTask, typename CollectRets,typename ...RestParams>
	requires invocable<ChildTask, SpreadNode,RestParams...>
		&&	 invocable< CollectRets, vector<invoke_result_t<ChildTask, SpreadNode,RestParams...>>&&>
invoke_result_t<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
SpreadCall(ChildTask ct, CollectRets cr, vector<SpreadNode> vs,RestParams&&... restParams) {
	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode, RestParams...>;
	vector<ChildTaskCallResultType> retparam;
	for (auto& node : vs)
		retparam.push_back(ct(node,forward<RestParams>(restParams)...));
	auto ret = cr(retparam);
	return ret;
}

template<typename SpreadNode, typename ChildTask,typename ...RestParams>
	requires invocable<ChildTask, SpreadNode,RestParams...>
vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>
SpreadCall(ChildTask ct, vector<SpreadNode> vs,RestParams&&... restParams) {
	return SpreadCall(ct,
	[](vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>> rets){return rets;},
		vs,	forward<RestParams>(restParams)...);
}

template<typename T>
	requires is_move_assignable_v<decay_t<T>>&& is_pod_v<T>
auto randomValue(T*des) {
	using DT = decay_t<T>;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i<int(sizeof(DT)); ++i) {
		auto rando_result=(rando() % 255) + 1;
		*(reinterpret_cast<unsigned char*>(des)) = rando_result;
	}
}
//addtional support for string
inline void randomValue(string* str);
template<typename T>
	requires is_move_assignable_v<decay_t<T>>&& is_pod_v<T> && is_constructible_v<T,decay_t<T>>
auto randomValue() {
	using DT = decay_t<T>;
	DT ret;
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	for (int i = 0; i<int(sizeof(DT)); ++i)
		*(reinterpret_cast<unsigned char*>(&ret)) = (rando() % 255) + 1;
	return ret;
}
 template<typename T>
	 requires is_move_assignable_v<decay_t<T>>&& is_pod_v<T>||is_same_v<decay_t<T>,string>
 auto randomValue(T* beg, int len) {
	 for (int i = 0; i < len; ++i)
		 randomValue(beg + i);
 }

 inline void randomValue(string* str) {
	 constexpr int len = 1000;
	 static char buffer[len];
	 static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	 const int strlen = (rando() % sizeof(buffer));
	 for (int i = 0; i<strlen; ++i)
		 buffer[i] = (rando() % ('z' - '0' + 1)) + '0';
	 buffer[strlen] = '0';
	 new(str) string(buffer);
 }
#endif //TEMPLATE_UTILITY_HEAD