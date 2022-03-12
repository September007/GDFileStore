#pragma once 
#include<string>
#include <string>
using std::string;
class Object {
public:
	string name;
	Object(const string& name = "") :name(name) {}
	Object(const string&& name) :name(name) {}
	bool operator==(const Object&)const = default;
	bool operator < (const Object&)const = default;
};
// the projection from a global object to a certain osd storage object is 
// pool->pg->obj
// different pool mean or born to different deploy rule
// here just have connection with pg and obj,but we still prepare the position for pool

class Snapid_t {
public:
	//when this been created
	time_t time_stamp = 0;
	Snapid_t(time_t t = 0) :time_stamp(t) {}
	//Snapid_t& operator+=(const Snapid_t& t) { this->time_stamp += t.time_stamp; return *this; }
	//Snapid_t operator+(const Snapid_t& t) { return Snapid_t(this->time_stamp +t.time_stamp ); }
	operator time_t() { return this->time_stamp; }
};
//
class Object_t {
public :
	string name;
};
//hash object
// pool is also a hash object
class HObject_t {
public:
	Object_t oid;
	Snapid_t snap;
private:
	uint32_t hash;
	bool max;
public:
	// ignoring
	static const int64_t POOL_META = -1;
	static const int64_t POOL_TEMP_START = -2; // and then negative
	int64_t pool = 0;
	static bool is_temp_pool(int64_t pool) {
		return pool <= POOL_TEMP_START;
	}
	static int64_t get_temp_pool(int64_t pool) {
		return POOL_TEMP_START - pool;
	}
	static bool is_meta_pool(int64_t pool) {
		return pool == POOL_META;
	}
};
//generation hash object
class GHObject_t {
public:
	HObject_t hobj;
	
};

class PageGroup {
public:
	string name;
	uint64_t pool = HObject_t::POOL_META;
	PageGroup(const string& name = "",uint64_t _pool=1) :name(name),pool(_pool) {}
	PageGroup(const string&& name) :name(name) {}
	bool operator==(const PageGroup&)const = default;
	bool operator < (const PageGroup&)const = default;
};