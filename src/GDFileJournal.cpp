#include<GDFileJournal.h>
#include<map>
#include<config.h>
#include<GDFileStore.h>
using fmt::format;
bool Journal::Mount( RocksKV* kv, GDFileStore* fs) {
	LOG_EXPECT_TRUE("Journal",kv->isDBLoaded());
	SetKV(kv);
	SetFS(fs);
	//@Todoo load some infomation from kv
	log_journal_head = GetConfig(fs->GetOsdName(), "log_journal_head", "FileStore");
	return true;
}
bool Journal::UnMount(){
	LOG_EXPECT_TRUE("Journal",kv->isDBLoaded());
	// flush some info into kv and quit
	//@Attention kv need to be shut down after this quit
	return true;
}
void Journal::SetPath(const string& path) {
	try {
		auto storagePath = filesystem::absolute(path).generic_string();
		this->storagePath = storagePath;
	}
	catch (std::exception& e) {
		LOG_ERROR("Journal", fmt::format("journal::setpath error [{}]", e.what()));
	}
}
void Journal::SetKV(RocksKV* kv) {
	this->kv = kv;
}
void Journal::SetFS(GDFileStore* kv) {
	this->fs = fs;
}
pair<bool,string> Journal::ReadObject(const GHObject_t& gh) {
	string s;
	auto uni = GetObjUniqueStrDesc(gh);
	//@dataflow journal check and read, class_journal will check kv_record when read object
	auto ret = kv->GetValue(format("{}{}",this->log_journal_head,uni), s);
	if (ret.ok()) {
		auto file = ReadFile(fmt::format("{}/{}", storagePath, uni));
		return make_pair(true, move(file));
	}
	else
		return make_pair(false, s);
}
void Journal::HandleWriteOperation(const Operation& wOpe, bool emengency) {
	_AddWriteOperation(wOpe);
	if (emengency) {
		_flushProcessor();
	}
}


void  Journal::HandleWriteOperation(const vector<Operation>& wOpe, bool emengency) {
	_AddWriteOperation(wOpe);
	if (emengency) {
		_flushProcessor();
	}
}



void Journal::_flushProcessor() {
	lock_guard lg(access_to_wOpes);
	map<GHObject_t, vector<Operation>> g2os;
	for (auto& ope : wOpes) {
		auto& os = g2os[ope.obj];
		if (os.size() == 0) {
			auto objData = Read(GetObjUniqueStrDesc(ope.obj));
			auto firstOPE = OperationWrapper::WriteWrapper(ope.obj, objData);
			os.push_back(move(firstOPE));
		}
		os.push_back(ope);
	}
		//clear it
	wOpes.clear();
	for (auto& [obj, v] : g2os) {
		int out_length = 0;
		auto ret = Operation::CombineOperationsForOneSameObject(v, out_length);
		auto buf = Operation::GetBufferFromSlices(ret, out_length);
		Write(GetObjUniqueStrDesc(obj), buf->universal_str());
		//@dataflow journal create
		//create journal here
		//just record journal object name
		//auto h = kv->GetHeader(obj);
		//kv->SetValue(format("{}{}",this->log_journal_head,h.to_string()), "");
		//log operation up to kv
		//kv->SetValue("", "");
		//@Todo : operation callback
	}
}

inline void Journal::_AddWriteOperation(const Operation& wOpe) {
	lock_guard lg(GetMutex<Journal*, string>(this, "wOpes"));
	wOpes.push_back(wOpe);
}

inline void Journal::_AddWriteOperation(const vector<Operation>& wOpe) {
	lock_guard lg(GetMutex<Journal*, string>(this, "wOpes"));
	wOpes.insert(wOpes.end(), wOpe.begin(), wOpe.end());
}
