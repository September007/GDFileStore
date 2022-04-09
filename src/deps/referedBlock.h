#pragma once
#include<assistant_utility.h>
#include<chrono>
#include<boost\container_hash\hash.hpp>
//#include<GDKeyValue.h>
using std::string;
class ReferedBlock {
public:
	//get from meta server
	//as a unique in range of whole storage system
	int64_t serial;
	int32_t refer_count;
	ReferedBlock(int64_t serial=0, int32_t refer_count=0) :serial(serial), refer_count(refer_count) {}
	static ReferedBlock getNewReferedBlock() {
		//this just create a increment serial for observe
		return ReferedBlock{ chrono::system_clock::now().time_since_epoch().count(),0 };
	}
	operator int64_t() { return serial; }
	auto GetES() { return make_tuple(&serial, &refer_count); }
	auto GetKey() { return make_tuple(&serial); }
	using Attr_Type = ReferedBlock;
};

//storage data object represented by refered block
class ObjectWithRB{
public:
	list<int64_t> serials_list;

	auto GetES() { return make_tuple(&serials_list); }
	auto GetKey() { return GetES(); }
};

//under root path, referedBlock managed as multi-demension array
//now assusm it's 4-demension,
//this only interset in referedBlock.serial
inline string GetReferedBlockStoragePath(const ReferedBlock &rb, string root_path) {
	auto s = rb.serial;
	ReleaseArea(s = boost::hash<decltype(rb.serial)>()(rb.serial));
	auto p1 = (s & 0xffff000000000000ll) >> 48;
	auto p2 = (s & 0x0000ffff00000000ll) >> 32;
	auto p3 = (s & 0x00000000ffff0000ll) >> 16;
	auto p4 = (s & 0x000000000000ffffll) >> 0;
	return fmt::format("{}/{}/{}/{}/{}.txt", root_path, p1, p2, p3, p4);
}
inline string ReadReferedBlock(int64_t serial, string root_path) {
	auto path = GetReferedBlockStoragePath(serial, root_path);
	return ReadFile(path);
}
inline string ReadObjectWithRB(ObjectWithRB orb,string root_path) {
	string ret;
	for (auto rb : orb.serials_list)
		ret += ReadReferedBlock(rb, root_path);
	return ret;
}
//check data.length is obligated with fs
inline void WriteReferedBlock(ReferedBlock rb, string root_path,string data) {
	auto path = GetReferedBlockStoragePath(rb, root_path);
	WriteFile(path, data);
}