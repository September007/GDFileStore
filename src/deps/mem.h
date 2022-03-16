#pragma once
#ifndef MEM_HEAD
#define MEM_HEAD
#include<concepts>
#include<unordered_set>
#include<unordered_map>
#include<fmt/format.h>
#include<assistant_utility.h>
#include<boost/functional/hash/hash.hpp>
// nuclear kill mosquito, but enjoy it,anyway it costing nothing
template<typename T>
auto GetSuitedCastedIntType(T* t) {
	if constexpr (sizeof(T*) == sizeof(int8_t))return reinterpret_cast<int8_t>(t);
	if constexpr (sizeof(T*) == sizeof(int16_t))return reinterpret_cast<int16_t>(t);
	if constexpr (sizeof(T*) == sizeof(int32_t))return reinterpret_cast<int32_t>(t);
	if constexpr (sizeof(T*) == sizeof(int64_t))return reinterpret_cast<int64_t>(t);
	else {
		static_assert(!std::is_same_v<T, T>);
		return t;
	}
}
using PointerIntType = decltype(GetSuitedCastedIntType(std::declval<void*>()));
// info of heap-allocated buffer 
struct HeapObjectInfo {
	void* heapLocation;
	int sz;
	const char* description;

	//malloc new,or new[]		and where
	const char* alloc_port;
	const char* alloc_where;
	//free delete,or delete[]	and where
	const char* release_port;
	const char* release_where;

	HeapObjectInfo(void* heapLocation, int sz, const char* description, const char* alloc_port, const char* alloc_where, const char* release_port, const char* release_where) :
		heapLocation(heapLocation), sz(sz), description(description), alloc_port(alloc_port), alloc_where(alloc_where), release_port(release_port), release_where(release_where) {
	}
	HeapObjectInfo() :heapLocation(0), sz(0), description(""), alloc_port(""), alloc_where(""), release_port(""), release_where("") {
	}
	string to_string()const {
		string ret = fmt::format(R"(
heapLocation:{:>12}
description :{}
alloc       :{:10} {}
release     :{:10} {})",
heapLocation, description, alloc_port, alloc_where, release_port, release_where);
		return ret;
	}
	bool operator < (const HeapObjectInfo& ot)const {
		// awkward,we don't neeed GetSuitedCastedIntType
		//it seem like a comparision function between type{void *}s 
		return heapLocation < ot.heapLocation;
	}
};
inline  bool operator ==(const HeapObjectInfo& l, const HeapObjectInfo& r) {
	return l.heapLocation == r.heapLocation && l.description == r.description && l.alloc_port == r.alloc_port && l.alloc_where == l.alloc_where && l.release_port == r.release_where && l.release_where == r.release_where;
}
struct HashForHeapObjectInfo {
	std::size_t operator()(const HeapObjectInfo& hoi)const noexcept {
		size_t seed;
		boost::hash_combine(seed, hoi.heapLocation);
		boost::hash_combine(seed, hoi.sz);
		boost::hash_combine(seed, hoi.description);
		boost::hash_combine(seed, hoi.alloc_port); boost::hash_combine(seed, hoi.alloc_where);
		boost::hash_combine(seed, hoi.release_port); boost::hash_combine(seed, hoi.release_where);
		return seed;
	}
};
class HeapRecorder final {
	HeapRecorder() {}
public:
	std::unordered_map<PointerIntType, HeapObjectInfo> records;
	std::unordered_set<HeapObjectInfo, HashForHeapObjectInfo> releases;
	auto record(void* pi, const HeapObjectInfo& hoi) {
		auto ret = records.insert(make_pair((PointerIntType)pi, hoi));
		LOG_EXPECT_TRUE("integrated", ret.second);
		return ret;
	}
	auto unrecord(void* pi, const char* release_port, const char* release_where) {
		auto r = records.find((PointerIntType)pi);
		LOG_EXPECT_TRUE("HeapRecorder", r != records.end());
		auto hoi = r->second;
		hoi.release_port = release_port;
		hoi.release_where = release_where;
		releases.insert(hoi);
		records.erase((PointerIntType)pi);
		return r;
	}
	void Report() {
		if (records.size()) {
			auto msg = fmt::format("detected unrecycled heap buffer {}X", records.size());
			GetLogger("integrated", false)->info(msg);
			std::cout << msg << std::endl;
			int cnt = 0;
			for (auto& [l, r] : records) {
				auto rmsg = fmt::format("NO.{}:{}\n", ++cnt, r.to_string());
				GetLogger("integrated", false)->info(rmsg);
				std::cout << rmsg;
			}
		}
	}
	// singleton pattern
	static inline HeapRecorder& instance() {
		static HeapRecorder hr;
		return hr;
	}

};

inline void* GD_malloc(int sz, const char* description, const char* alloc_where) {
	auto ret = malloc(sz);
	HeapRecorder::instance().record(ret, HeapObjectInfo(ret, sz, description, "GD_malloc", alloc_where, "", ""));
	return ret;
}
#endif //MEM_HEAD