#pragma once 
#include<string>
#include<vector>
#include<assistant_utility.h>
#include<boost/functional/hash.hpp>
#include<fmt/format.h>
#define declare_default_cmp_operator(cls)			   \
bool operator <(const cls&)const = default;	   \
bool operator ==(const cls&)const = default;	   \
bool operator !=(const cls&)const = default;

using std::string;
class Object {
public:
	string name;
	Object(const string& name = "") :name(name) {}
	declare_default_cmp_operator(Object)
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
	declare_default_cmp_operator(Snapid_t)
	//Snapid_t& operator+=(const Snapid_t& t) { this->time_stamp += t.time_stamp; return *this; }
	//Snapid_t operator+(const Snapid_t& t) { return Snapid_t(this->time_stamp +t.time_stamp ); }
	operator time_t() { return this->time_stamp; }
	auto GetES()const  { return make_tuple(&time_stamp); }
};
//
class Object_t {
public:
	string name;
	Object_t(const string name = "") :name(name) {}
	declare_default_cmp_operator(Object_t)
	auto GetES()const  { return make_tuple(&name); }
};
//hash object
// pool is also a hash object
class HObject_t {
public:
	Object_t oid;
	Snapid_t snap;
	HObject_t(Object_t oid = Object_t(), Snapid_t snap = Snapid_t()) :oid(oid) {}
	declare_default_cmp_operator(HObject_t)
	/*
	*  don't wanna use these
	*/
private:
	// genrated by Hash<string>()(oid.name)
	//[[maybe_unused]]
	uint32_t hash = 0;
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
	auto GetES()const  { return make_tuple(&oid, &snap, &hash, &max); }
};

using gen_t = int64_t;
struct shard_id_t {
	int8_t id;

	shard_id_t() : id(0) {}
	declare_default_cmp_operator(shard_id_t)
	explicit shard_id_t(int8_t _id) : id(_id) {}

	operator int8_t() const { return id; }

	static inline shard_id_t NO_SHARD() { return shard_id_t(0); };
	auto GetES()const  { return make_tuple(&id); }
};

namespace boost {
//@follow difinition of shard_id_t
	inline size_t hash_value(const shard_id_t&sh) {
		return boost::hash_value(sh.id);
	}
}
//generation hash object, or the global balah object
class GHObject_t {
public:
	HObject_t hobj;
	gen_t generation;
	shard_id_t shard_id;
	//@new point out the owner of this obj 
	string owner = "default-user";
	// these implement is needed
	GHObject_t(const HObject_t& hobj = HObject_t(), gen_t generation = 0, shard_id_t shard_id = shard_id_t::NO_SHARD()) :
		hobj(hobj), generation(generation), shard_id(shard_id) {
	}
	//@Follow 
	bool operator<(const GHObject_t& obj)const {
		return this->hobj.oid.name < obj.hobj.oid.name ||
			this->hobj.oid.name == obj.hobj.oid.name && this->generation < obj.generation;
	}
	bool operator==(const GHObject_t& obj)const {
		return this->hobj.oid.name == obj.hobj.oid.name && this->generation == obj.generation
			&& this->shard_id == obj.shard_id && this->owner == obj.owner;
	}
	bool operator!=(const GHObject_t& obj)const {
		return !operator==(obj);
	}

	// get literal description of object
	//@Follow:GHObject_t definition
	string GetLiteralDescription(GHObject_t const& ghobj);
	auto GetES()const  {
		return make_tuple(&hobj, &generation, &shard_id, &owner);
	}
};
//@follow GHObject_t
struct HashForGHObject_t {
	size_t operator()(const GHObject_t& ghobj)const {
		size_t seed = 0;
		boost::hash_combine(seed, ghobj.owner);
		boost::hash_combine(seed, ghobj.generation);
		boost::hash_combine(seed, ghobj.shard_id.id);
		boost::hash_combine(seed, ghobj.hobj.pool);
		boost::hash_combine(seed, ghobj.hobj.snap.time_stamp);
		boost::hash_combine(seed, ghobj.hobj.oid.name);
		return seed;
	}
};
// Page Group
class PageGroup {
public:
	string name;
	uint64_t pool = HObject_t::POOL_META;
	PageGroup(const string& name = "", uint64_t _pool = 1) :name(name), pool(_pool) {}
	PageGroup(const string&& name) :name(name) {}
	declare_default_cmp_operator(PageGroup)
	auto GetES()const  { return make_tuple(&name, &pool); }
};
//@follow definition of GHObject_t
//get the unique decription str for a ghobj
inline auto GetObjUniqueStrDesc(GHObject_t const& ghobj) {
	auto str = fmt::format("{}{:0>}{}{}{:0>}{:0>}",
		ghobj.owner, ghobj.hobj.pool, ghobj.hobj.oid.name,ghobj.hobj.snap.time_stamp,ghobj.generation, ghobj.shard_id);
	return str;
}


// basic operations only have the below two
enum class OperationType {
	Insert,
	Delete
};
// operation state  for detail see file design.vsdx
enum class OperationState {
	//for write
	wrapped, inJournalQueue, inJournalCache, inFS
	//for read
};
// file anchor like std::fstream::begin
enum class FileAnchor {
	begin = 1,
	end = 2
};
// point out a point position
class FilePos {
public:
	FileAnchor fileAnchor;
	int offset;
	FilePos(int offset=0, FileAnchor fileAnchor = FileAnchor::begin)
		: offset(offset)
		, fileAnchor(fileAnchor) {
	}
	FilePos(const FilePos&) = default;
	declare_default_cmp_operator(FilePos)
	auto GetES()const  { return make_tuple(&fileAnchor, &offset); }
};
class Operation {
public:
	// operation type, insert or delete
	OperationType operationType;
	/* operation data
	* when in delete, only its start and end matter ,and the span of being delete is end-start(as Slice.GetSize())
	* @contract_1  start==0,see LOG_EXEPECT below
	* @contract_2 data.size()+filePos<=objLength
	*/
	Slice data;
	// operation related object
	GHObject_t obj;
	// operation begin point
	FilePos filePos;
	//use memeber default
	Operation() {}
	Operation(const GHObject_t& ghobj, OperationType operationType, Slice data, FilePos filePos)
		: obj(ghobj)
		, operationType(operationType)
		, data(data)
		, filePos(filePos) {
	}
	bool operator ==(const Operation& ope)const {
		return this->operationType==ope.operationType&&obj == ope.obj && data == ope.data && filePos == ope.filePos;
	}
	// combine writes on the same object
	static vector<Slice> CombineOperationsForOneSameObject(const vector<Operation>& operations,
		int& out_totalLength);
	static vector<Slice> CombineOperationsForOneSameObject(shared_ptr<buffer> objData,
		const vector<Operation>& operations, int& out_totalLength);
	static shared_ptr<buffer> GetBufferFromSlices(const vector<Slice>& slices, int indicated_length = 0);
	static int GetFilePos(const Operation ope, int endPos);
	// support for serialize
	auto GetES()const  { return make_tuple(&operationType, &data, &obj, &filePos); }
};

class OperationWrapper {
public:
	// overwrite
	static Operation WriteWrapper(GHObject_t const& ghobj, const string& data) {
		return Operation(ghobj, OperationType::Insert, Slice(data), FilePos(0, FileAnchor::begin));
	}
	// insert a string
	static Operation InsertWrapper(GHObject_t const& ghobj, const string& data, int pos) {
		return Operation(ghobj, OperationType::Insert, Slice(data), FilePos(pos, FileAnchor::begin));
	}
	// delete all
	static Operation DeleteWrapper(GHObject_t const& ghobj, int SpanBegin, int SpanEnd) {
		return Operation(ghobj, OperationType::Delete, Slice(nullptr, SpanBegin, SpanEnd), FilePos(0, FileAnchor::begin));
	}
	// delete span
	//static Operation DeleteWrapper(GHObject_t const& ghobj, int SpanBegin, int SpanEnd) {
	//	return Operation(OperationType::Delete, Slice(nullptr, SpanBegin, SpanEnd), FilePos(0, FileAnchor::begin));
	//}
	// append at end
	static Operation AppendWrapper(GHObject_t const& ghobj, const string& data) {
		return Operation(ghobj, OperationType::Insert, Slice(data), FilePos(0, FileAnchor::end));
	}
};
//@Todo.t2 use this!!
// each operation have a callback
using OperationCallBackType = std::function<void(Operation*)>;


using opeIdType = string;

//@new operation
class WOPE {
public:
	enum class opetype {
		Insert,Delete
	};
	opetype type;
	GHObject_t ghobj;
	// for the block support
	int block_num;
	string block_data;
	//support serialize
	auto GetES()const  { return make_tuple(&type, &ghobj, &block_num, &block_data); }
};

class ROPE {
public:
	GHObject_t ghobj;
	//serial numbers of blocks 
	vector<int> blocks;
	ROPE(const GHObject_t& gh, const std::vector<int> &blocks) :ghobj(gh), blocks(blocks) {}
	auto GetES()const  { return make_tuple(&ghobj, &blocks); }
};
//result of read ope
class ROPE_Result {
public:
	GHObject_t ghobj;
	//serial numbers of blocks 
	vector<int> blocks;
	vector<string> datas;
	ROPE_Result(const GHObject_t& gh, const std::vector<int>& blocks,const vector<string>&datas)
		:ghobj(gh), blocks(blocks),datas(datas) {}
	ROPE_Result(const ROPE_Result&) = default;
};
//@follow definition of WOPE
inline opeIdType GetOpeId(const WOPE& wope) {
	//add time_stamp
	auto time_stamp = chrono::system_clock::now().time_since_epoch().count();
	return fmt::format("{}:{}:{}:{}", int(wope.type), GetObjUniqueStrDesc(wope.ghobj), wope.block_num, time_stamp);
}
//@follow definition of ROPE
inline opeIdType GetOpeId(const ROPE& rope) {
	//add time_stamp
	auto time_stamp = chrono::system_clock::now().time_since_epoch().count();
	return fmt::format("{}:{}", GetObjUniqueStrDesc(rope.ghobj), time_stamp);
}