

#include<gtest/gtest.h>
#include<iostream>
#include<concepts>
#include<unordered_set>
#include<fmt/format.h>
#include<assistant_utility.h>
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
	//code location balahbalah
	const char* description;
	//malloc new,or new[]
	const char* alloc_source;
	HeapObjectInfo(void* heapLocation, int sz, const char* description, const char* alloc_source) :
		heapLocation(heapLocation), sz(sz), description(description), alloc_source(alloc_source) {
	}
	string to_string()const {
		string ret = fmt::format(R"(
heapLocation:{:>12}
description :{}
alloc_source:{})", heapLocation, description, alloc_source);
		return ret;
	}
	bool operator < (const HeapObjectInfo& ot)const {
		// awkward,we don't neeed GetSuitedCastedIntType
		//it seem like a comparision function between type{void *}s 
		return heapLocation < ot.heapLocation;
	}
};
inline  bool operator ==(const HeapObjectInfo& l, const HeapObjectInfo& r) {
	return l.heapLocation == r.heapLocation && l.description == r.description && l.alloc_source == r.alloc_source;
}
struct HashForHeapObjectInfo {
	std::size_t operator()(const HeapObjectInfo& hoi)const noexcept {
		auto _hl = std::hash<decltype(hoi.heapLocation)>{}(hoi.heapLocation);
		auto _sz = std::hash<decltype(hoi.sz)>{}(hoi.sz);
		auto _ds = std::hash<decltype(hoi.description)>{}(hoi.description);
		auto _as = std::hash<decltype(hoi.alloc_source)>{}(hoi.alloc_source);
		return _hl ^ (_sz << 1) ^ (_ds << 2) ^ (_as << 3);
	}
};
class HeapRecorder {
public:
	std::unordered_set<HeapObjectInfo,HashForHeapObjectInfo> record;
	void Report() {
		if (record.size()) {
			auto msg = fmt::format("detected unrecycled heap buffer {}X", record.size());
			GetLogger("integrat", false)->info(msg);
			std::cout << msg << std::endl;
			int cnt = 0;
			for (auto& r : record) {
				auto rmsg = fmt::format("NO.{}:{}\n", ++cnt, r.to_string());
				GetLogger("integrat", false)->info(rmsg);
				std::cout << rmsg;
			}
		}
	}
};

inline HeapRecorder* GetHeapRecorder() {
	static HeapRecorder hr;
	return &hr;
}
inline void* GD_malloc(int sz, const char* description) {
	auto ret = malloc(sz);
	GetHeapRecorder()->record.emplace(HeapObjectInfo(ret, sz, description, "GD_malloc"));
	return ret;
}
int main() {

	auto cv = (char*)GD_malloc(10, "test");

#ifdef __SANITIZE_ADDRESS__
	printf("Address sanitizer enabled\n");
#else
	printf("Address sanitizer not enabled\n");
#endif
	::testing::InitGoogleTest();
	auto ret = RUN_ALL_TESTS();
#ifdef __linux__
	std::getchar();
#endif
	GetHeapRecorder()->Report();
	return 0;
}