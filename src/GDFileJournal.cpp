#include<GDFileJournal.h>
#include<map>
#include<config.h>

void Journal::Init(const string& path, RocksKV* kv) {
	SetPath(path);
	SetKV(kv);
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
	map<GHObject_t, vector<Operation>> g2os;
	for (auto& ope : wOpes) {
		auto& os = g2os[ope.obj];
		if (os.size() == 0) {
			auto objData = Read(GetObjDirHashInt(ope.obj));
			auto firstOPE = OperationWrapper::WriteWrapper(ope.obj, objData);
			os.push_back(move(firstOPE));
		}
		os.push_back(ope);
	}
	for (auto& [obj, v] : g2os) {
		int out_length = 0;
		auto ret = Operation::CombineOperationsForOneSameObject(v, out_length);
		auto buf = Operation::GetBufferFromSlices(ret, out_length);
		Write(GetObjDirHashInt(obj), buf->universal_str());
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
