#include<GDKeyValue.h>
#include<GDMutex.h>
#include<config.h>
using key_type = RocksKV::key_type;
using value_type= RocksKV::value_type;
bool RocksKV::LoadDB() {
	if (KVBase::is_db_loaded)return true;
	if (!filesystem::is_directory(storagePath))
		filesystem::create_directories(storagePath);
	rocksdb::Options options;
	options.create_if_missing = true;
	rocksdb::Status status = rocksdb::DB::Open(options, storagePath, &this->db);
	if (!this->db) {
		GetLogger("RocksKV")->error("create RocksKV[{}] failed.{}:{}", storagePath, __FILE__, __LINE__);
		return false;
	}
	else
		return KVBase::is_db_loaded = true;
}

bool RocksKV::UnLoadDB() {
	if (KVBase::is_db_loaded) {
		KVBase::is_db_loaded = false;
		this->db->Close();
		this->db = nullptr;
	}
	return true;
}

vector<pair<key_type, value_type>> RocksKV::GetMatchPrefix(const string& prefix) {
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

bool RocksKV::beginWith(const string& pre, const string& str) {
	if (pre.size() > str.size())return false;
	return str.substr(0, pre.size()) == pre;
}

vector<rocksdb::Slice> RocksKV::_turn_Slice(const vector<std::string>& strs) {
	vector<rocksdb::Slice> ret(strs.size());
	for (int i = 0; i < strs.size(); ++i)
		ret[i] = strs[i];
	return ret;
}

Header Header::GetNewHeader(RocksKV* kv) {
	// modify kv::header_lock is thread mutual exclusive
	std::lock_guard lg(GetMutex<RocksKV*>(kv,mutex_enum::header_lock));
	string head_count;
	auto ret = kv->GetValue("header_count", head_count);
	if (!ret.ok()) {
		//read config from point-outed config file
		auto default_header_count = GetConfig("KV", "header_count").get<string>();
		LOG_INFO("KV", fmt::format("KV[{}] missing header_count, create as[{}] at [{}:{}] "
			, long(kv), default_header_count, __FILE__, __LINE__));
		head_count = default_header_count;
	}
	auto hc = stoull(head_count);
	head_count = std::to_string(hc + 1);
	kv->SetValue("header_count", head_count);
	return Header(hc);
}
