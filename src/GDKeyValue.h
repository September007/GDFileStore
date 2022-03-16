#pragma once
#ifndef GDKEYVALUE_HEAD
#define GDKEYVALUE_HEAD
#include<rocksdb/db.h>
#include<spdlog/spdlog.h>
#include<assistant_utility.h>
#include<map>
#include<vector>
#include<boost/version.hpp>
using std::string;
using std::vector;
using std::map;

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
};

class RocksKV :public KVBase {
public:
	rocksdb::DB* db = nullptr;
	string storagePath = "";
	RocksKV(const string& storagePath="") :storagePath(filesystem::absolute(storagePath).string()) {}
	~RocksKV() {
		if (db) {
			db->Close();
			db = nullptr;
		}
	}
	//return true if load success
	bool LoadDB()override {
		if (KVBase::is_db_loaded)return true;
		if (!filesystem::is_directory(storagePath))
			filesystem::create_directories(storagePath);
		rocksdb::Options options;
		options.create_if_missing = true;
		rocksdb::Status status = rocksdb::DB::Open(options, storagePath, &this->db);
		if (!this->db)
		{
			GetLogger("RocksKV")->error("create RocksKV[{}] failed.{}:{}", storagePath, __FILE__, __LINE__);
			return false;
		}
		else 
			return KVBase::is_db_loaded = true;
	}
	bool UnLoadDB()override{
		if (KVBase::is_db_loaded) {
			KVBase::is_db_loaded = false;
			this->db->Close();
			this->db = nullptr;
		}
		return true;
	} 
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
	vector<pair<key_type, value_type>> GetMatchPrefix(const string& prefix) {
		auto it = db->NewIterator(rocksdb::ReadOptions());
		vector<pair<key_type, value_type>> ret;
		for (it->SeekForPrev(prefix); it->Valid(); it->Next()) {
			if (beginWith(prefix, it->key().ToString()))
				ret.push_back({ string(it->key().ToString()),string(it->value().ToString()) });
		}
		if (!it->status().ok()) {
			std::cout << it->status().ToString();
		}
		//dont forget to delete it
		delete it;
		return ret;
		
	}

private :
	static bool beginWith(const string& pre, const string& str) {
		if (pre.size() > str.size())return false;
		return str.substr(0, pre.size()) == pre;
	}
	// note slice does not hold data,it just refer,so keep original data alive
	static vector<rocksdb::Slice> _turn_Slice(const vector<std::string>& strs){
		vector<rocksdb::Slice> ret(strs.size());
		for (int i = 0; i < strs.size(); ++i)
			ret[i] = strs[i];
		return ret;
	}
};

#endif //GDKEYVALUE_HEAD