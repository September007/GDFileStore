#pragma once 
#include<string>
#include<boost/functional/hash.hpp>
#include<fmt/format.h>
using std::string;
class Object {
public:
	string name;
	Object(const string& name = "") :name(name) {}
	bool operator==(const Object&)const = default;
	bool operator < (const Object&)const = default;
};
// the projection from a global object to a certain osd storage object is 
// pool->pg->obj
// different pool mean or born to different deploy rule


// copied
#define CEPH_SNAPDIR ((__u64)(-1))  /* reserved for hidden .snap dir */
#define CEPH_NOSNAP  ((__u64)(-2))  /* "head", "live" revision */
#define CEPH_MAXSNAP ((__u64)(-3))  /* largest valid snapid */

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
	HObject_t(Object_t oid=Object_t(), Snapid_t snap=Snapid_t()) :oid(oid) {}
	/*
	*  don't wanna use these
	*/
private:
	// genrated by Hash<string>()(oid.name)
	//[[maybe_unused]]
	uint32_t hash=0;
	//[[maybe_unused]]
	bool max = false;
public:
	// ignoring
	static const int64_t POOL_META = -1;
	static const int64_t POOL_TEMP_START = -2; // and then negative
	int64_t pool = 0;
	//default counts of pg in a pool
	static constexpr int default_pg_numbers = 100;
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

using gen_t = int64_t;
struct shard_id_t {
	int8_t id;

	shard_id_t() : id(0) {}
	explicit shard_id_t(int8_t _id) : id(_id) {}

	operator int8_t() const { return id; }

	static inline shard_id_t NO_SHARD() { return shard_id_t(0); };
};
//generation hash object, or the global balah object
class GHObject_t {
public:
	HObject_t hobj;
	gen_t generation;
	shard_id_t shard_id;
	//@new point out the owner of this obj 
	string owner = "default-user";
	// these implement is needed
	GHObject_t(const HObject_t& hobj = HObject_t(), gen_t generation=0, shard_id_t shard_id=shard_id_t::NO_SHARD()) :
		hobj(hobj), generation(generation), shard_id(shard_id) {
	}
	bool operator<(const GHObject_t ot)const {
		return this->hobj.oid.name < ot.hobj.oid.name ||
			this->hobj.oid.name == ot.hobj.oid.name && this->hobj.pool < ot.hobj.pool;
	};
	bool operator==(const GHObject_t& ot)const {
		return this->hobj.oid.name == ot.hobj.oid.name && this->hobj.pool == ot.hobj.pool;
	}
	// get literal description of object
	//@Follow:GHObject_t definition
	string GetLiteralDescription(GHObject_t const& ghobj);
};
struct HashForGHObject_t {
	size_t operator()(const GHObject_t&ghobj)const{
		size_t seed = 0;
		boost::hash_combine(seed, ghobj.hobj.oid.name);
		boost::hash_combine(seed, ghobj.hobj.pool);
		return seed;
	}
};
// Page Group
class PageGroup {
public:
	string name;
	uint64_t pool = HObject_t::POOL_META;
	PageGroup(const string& name = "",uint64_t _pool=1) :name(name),pool(_pool) {}
	PageGroup(const string&& name) :name(name) {}
	bool operator==(const PageGroup&)const = default;
	bool operator < (const PageGroup&)const = default;
};

inline auto GetObjDirHashInt(GHObject_t const& ghobj) {
	return std::to_string(std::hash<string>()(ghobj.hobj.oid.name));
}