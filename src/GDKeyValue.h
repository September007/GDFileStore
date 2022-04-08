#pragma once
#ifndef GDKEYVALUE_HEAD
#define GDKEYVALUE_HEAD
#include<rocksdb/db.h>
#include<spdlog/spdlog.h>
#include<assistant_utility.h>
#include<map>
#include<vector>
#include<boost/version.hpp>
#include<GDMutex.h>
#include<object.h>
#include<forward_declare.h>
#include<object.h>
using std::string;
using std::vector;
using std::map;

/* Header , simplify key's length
* eg: usr0.obj0001.attr0: val0	usr0.obj0001.attr1: val1	usr0.obj0001.attr2: val2
* could be k001.attr0: val0	k001.attr1: val1 k001.attr2: val2
*/
class Header {
public:
	uint32_t key;
	explicit Header(uint32_t k=0) :key(k) {}
	bool operator<(const Header&)const = default;
	std::strong_ordering operator<=>(const Header&)const = default;
	//@follow definition of Header
	string to_string()const { return fmt::format("h_{}", key); }
	static Header GetNewHeader(RocksKV* kv);
	//@follow definition of Header
BOOST_NOINLINE	static Header FromTo_string(const string& header_str);
	auto GetES()const  { return make_tuple(&key); }
};
// kv interface
// crud boy,ewwww
//};
using KVStatus = rocksdb::Status;
class KVBase {
public:
	using key_type = std::string;
	using value_type = std::string;
	//is kv db loaded
	bool is_db_loaded = false;
	virtual bool LoadDB() = 0;
	virtual bool UnLoadDB() = 0;
	virtual ~KVBase() {};
	virtual KVStatus GetValue(const key_type& key,value_type&)=0;
	//only return those who have record
	virtual vector<KVStatus> GetValues(const vector<key_type>& keys, vector<value_type>& retVals)=0;
	virtual KVStatus SetValue(const key_type& key, const value_type&)=0;
	virtual vector<KVStatus> SetValues(const vector<pair<key_type, value_type>>& vals)=0;
	virtual KVStatus RemoveKey(const key_type& key)=0;
	virtual vector<KVStatus> RemoveKeys(const vector<key_type>& keys)=0;

	auto isDBLoaded(){return is_db_loaded;}
};

class RocksKV :public KVBase {
public:
	rocksdb::DB* db = nullptr;
	string storagePath = "";
	//here if storagePath is empty, gcc::filesystem::absolute will cause error
	RocksKV(const string& storagePath="/tmp/rockskv_tmp") :storagePath(filesystem::absolute(storagePath).string()) 
	//,type_headers() 
	{ }
	~RocksKV() {
		if (db) {
			db->Close();
			db = nullptr;
		}
	}
	//return true if load success
	bool LoadDB()override;
	bool UnLoadDB()override;
	void SetPath(const string &path){storagePath=path;}
	KVStatus GetValue(const key_type& key, value_type&val) override{
		auto status = db->Get(rocksdb::ReadOptions(), key, &val);
		return status;
	}
	vector<KVStatus> GetValues(const vector<key_type>& keys, vector<value_type>& retVals)override {
		auto slices = _turn_Slice(keys);
		auto ret = db->MultiGet(rocksdb::ReadOptions(),slices, &retVals);
		return ret;
	}
	KVStatus SetValue(const key_type& key, const value_type&val)override {
		auto ret = db->Put(rocksdb::WriteOptions(), key, val);
		return ret;
	}
	vector<KVStatus> SetValues(const vector<pair<key_type,value_type>>& vals)override {
		auto ret = vector<KVStatus>(vals.size());
		for (int i = 0; i < vals.size(); ++i)
			ret[i] = SetValue(vals[i].first, vals[i].second);
		return ret;
	}
	map<key_type, KVStatus> SetValues(const map<key_type, value_type>& m) {
		map<key_type, KVStatus> ret;
		for (auto& p : m)
			ret[p.first] = SetValue(p.first, p.second);
		return ret;
	}
	KVStatus RemoveKey(const key_type& key)override {
		return db->Delete(rocksdb::WriteOptions(), key);
	}
	vector<KVStatus> RemoveKeys(const vector<key_type>& keys) {
		auto ret = vector<KVStatus>(keys.size());
		for (int i = 0; i < keys.size(); ++i)
			ret[i] = RemoveKey(keys[i]);
		return ret;
	}
	vector<pair<key_type, value_type>> GetMatchPrefix(const string& prefix);

	//
	std::map<GHObject_t, Header> objCacheHeader;
	std::map<Header,GHObject_t > reverse_objCacheHeader;
	//if this obj doesnot have a header, will create it
	//create ky-pair: GetObjUniqueStrDesc(gh) \to header ,  and  header \to GetObjUniqueStrDesc(gh)
	Header GetHeader(const GHObject_t& gh);
	void EraseHeader(const GHObject_t& gh);
	GHObject_t GetObj (const Header& h);
	void EraseObj(const Header& h);

	//@new for the object attributes storage

	//get the type_header
	using header_key = decltype(declval<type_info>().name());
	std::map<header_key, string> type_headers;
	std::mutex access_to_type__headers;
	template<typename T>
	header_key GetTypeHeader_Key() {
		using DT = decay_t<T>;
		return typeid(DT).name();
	}
	template<typename T=int>
	string GetTypeHeader();

	template<typename T=int>
	string Get_Key(T t) {
		using DT = decay_t<T>;
		if constexpr (is_arithmetic_v<DT>||is_enum_v<DT>){
			return as_string(t);
		}else
		if constexpr (requires (DT dt) { dt.GetKey(); })
		{
			auto key = t.GetKey();
			return as_string(key);
		}
		else if constexpr (requires(DT dt) {	dt.GetES();	})
		{
			auto key = t.GetES();
			return as_string(key);
		}
		else if constexpr (is_same_v<T, T>) {
			static_assert(!is_same_v<T, T>, "");
		}
	}
	template<typename T=int>
	void SetAttr(T& t,T::Attr_Type attr) {
		auto key = Get_Key(t);
		auto val = as_string(attr);
		SetValue(key, val);
	}
	template<typename T=int>
	T::Attr_Type GetAttr(T& t) {
		auto key = Get_Key(t);
		string attr;
		GetValue(key, attr);
		return from_string<T::Attr_Type>(attr);
	}
private :
	Header GetNewHeader();
	static bool beginWith(const string& pre, const string& str);
	// note slice does not hold data,it just refer,so keep original data alive
	static vector<rocksdb::Slice> _turn_Slice(const vector<std::string>& strs);
};


template<typename T>
inline string RocksKV::GetTypeHeader() {
	using DT = decay_t<T>;
	header_key hk = GetTypeHeader_Key<DT>();
	auto p = type_headers.find(hk);
	if (p != type_headers.end())
		return p.second;
	else {
		std::unique_lock lg(access_to_type__headers);
		//default header
		string default_header = typeid(DT).raw_name();
		//if find RocksKV[default_header],use it or use default_header
		auto header = GetconfigOverWrite(default_header, "universal_header", "RocksKV", default_header);
		type_headers[hk] = header;
		return header;
	}
}
#endif //GDKEYVALUE_HEAD
