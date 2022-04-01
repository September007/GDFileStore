#include<GDKeyValue.h>
#include<GDMutex.h>
#include<config.h>
#include<string>
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
	//dont forget to delete it
	delete it;
	return ret;

}

Header RocksKV::GetNewHeader() {
	return Header::GetNewHeader(this);
}

Header RocksKV::GetHeader(const GHObject_t& gh) {
	auto cache = objCacheHeader.find(gh);
	if (cache != objCacheHeader.end())return cache->second;
	string s;
	auto objuni = GetObjUniqueStrDesc(gh);
	auto ret = this->GetValue(objuni, s);
	if (ret.ok()) {
		buffer b = s;
		auto h = Read<Header>(b);
		objCacheHeader[gh] = h;
		return h;
	}
	else {
		auto h = GetNewHeader();
		this->SetValue(objuni, h.to_string());
		this->SetValue(h.to_string(), objuni);
		objCacheHeader[gh] = h;
		reverse_objCacheHeader[h] = gh;
		return h;
	}
}

void RocksKV::EraseHeader(const GHObject_t& gh) {
	auto h = objCacheHeader.find(gh);
	auto head = h->second;
	if (h != objCacheHeader.end()) {
		objCacheHeader.erase(h);
		reverse_objCacheHeader.erase(head);
	}
	this->RemoveKey(GetObjUniqueStrDesc(gh));
	//even if head is "", remove won't rise error
	this->RemoveKey(head.to_string());
}

GHObject_t RocksKV::GetObj(const Header& h) {
	auto cache = reverse_objCacheHeader.find(h);
	if (cache != reverse_objCacheHeader.end())return cache->second;
	string s;
	auto head = h.to_string();
	auto ret = this->GetValue(head, s);
	if (ret.ok()) {
		buffer b = s;
		auto gh = Read<GHObject_t>(b);
		reverse_objCacheHeader[h] = gh;
		objCacheHeader[gh] = h;
		return gh;
	}
	else {
		// new header should come from GetHeader,not here
		LOG_ERROR("Header", fmt::format("try to get the projected object cfrom a Header[],but failed", h.to_string()));
		return GHObject_t();
	}
}

void RocksKV::EraseObj(const Header& h) {
	auto f = reverse_objCacheHeader.find(h);
	auto gh = f->second;
	if (f != reverse_objCacheHeader.end()) {
		reverse_objCacheHeader.erase(f);
		objCacheHeader.erase(gh);
	}
	//even if gh is default, it's fine
	this->RemoveKey(GetObjUniqueStrDesc(gh));
	this->RemoveKey(h.to_string());
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
	int hc;
	auto ret = kv->GetValue("header_count", head_count);
	if (!ret.ok()) {
		//read config from point-outed config file
		auto default_header_count = GetConfig("RocksKV", "header_count").get<int>();
		LOG_INFO("KV", fmt::format("RocksKV[{}] missing header_count, create as[{}] "
			,  long long(kv), default_header_count));
		hc = default_header_count;
	}
	else
		hc = stoull(head_count);
	head_count = std::to_string(hc + 1);
	kv->SetValue("header_count", head_count);
	return Header(hc);
}

Header Header::FromTo_string(const string& header_str) {
	return Header(std::stoul(header_str.substr(2)));
}
