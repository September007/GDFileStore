#include"deps.h"
#include"GDFileStore.h"
using  file_object_data_type=GDFileStore::file_object_data_type;
using KVStore_type=GDFileStore::KVStore_type;

GDFileStore::GDFileStore(const string& StorePath,const string &fsName) :
	path(std::filesystem::absolute(StorePath).generic_string()),
	fsname(fsName),
	kv(),
	journal(),
	timercaller(0)
{
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	//set submodules root
	kv.SetPath( GetKVRoot());
	journal.SetPath( GetJournalRoot());
}
GDFileStore::~GDFileStore() { 
	timercaller.shutdown();
}
bool GDFileStore::HandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds) {
	if (osds.size() == 0) {
		LOG_ERROR("server", fmt::format("get unexpected 0 osds"));
		return false;
	}
	auto &primaryOsd = osds[0];
	if (primaryOsd.name == this->GetOsdName())
		return PrimaryHandleWriteOperation(wope, osds);
	else
		return ReplicaHandleWriteOperation(wope, osds);
}
bool GDFileStore::PrimaryHandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds) {
	//
	this->journal.HandleWriteOperation(wope, true);
	return true;
}
bool GDFileStore::ReplicaHandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds) {
	auto info = GetInfo();
	for (auto &o : osds)
		if (info.name == o.name) goto handle;
	return false;
handle:
	journal.HandleWriteOperation(wope, true);
	return true;
}
void GDFileStore::flushJouranl() {
	lock_guard lg(journal.access_to_wOpes);
	//@dataflow journal read
	auto log_head = journal.log_journal_head;
	auto js = kv.GetMatchPrefix(log_head);
	for (auto &[k,v] : js) {
		auto head = k.substr(log_head.size());
		auto h = Header::FromTo_string(head);
		auto gh = kv.GetObj(h);
		auto unistr = GetObjUniqueStrDesc(gh);
		auto journal_ret = journal.ReadObject(gh);
		if (journal_ret.first)
			WriteFile(GetGHObjectStoragePath(gh), journal_ret.second);
		else {
			LOG_ERROR("Journal", 
				fmt::format("read journal[{}] for obj[{}] in kv,but journal doesn;t have it's content", k,unistr));
			//@todo callback
		}
		//@dataflow journal remove
		kv.EraseHeader(gh);
	}
}
auto GDFileStore::_WriteJournal(GHObject_t obj, const string& content) {
	auto objDirHashInt = GetObjUniqueStrDesc(obj);
	auto ret = journal.Write(objDirHashInt, content);
	if (!ret) {
		GetLogger("Write")->error("write journal {} failed.{}:{}", objDirHashInt, __FILE__, __LINE__);
		return false;
	}
	//value set empty
	kv.SetValue("journal::write_" + GetGHObjectStoragePath(obj), objDirHashInt);
	return true;
};
auto GDFileStore::flushWriteJournal() {
	auto allWriteJournal = kv.GetMatchPrefix("journal::write_");
	for (auto& writeJournal : allWriteJournal) {
		auto objDir = writeJournal.first.substr(string("journal::write_").size());
		auto objContent = journal.Read(writeJournal.second);
		WriteFile(objDir, objContent);
	}
}
inline PageGroup GDFileStore::GetPageGroupFroGHOBJ(const GHObject_t& ghobj) const {
	PageGroup ret;
	ret.pool = ghobj.hobj.pool;
	// @Todo :may need visit metadata 
	auto pgs = HObject_t::default_pg_numbers;
	auto num = HashForGHObject_t()(ghobj) % pgs;
	ret.name = fmt::format("{:>04}", num);
	return ret;
}
string GDFileStore::GetGHObjectStoragePath(const GHObject_t& ghobj) const {
	auto pg = GetPageGroupFroGHOBJ(ghobj);
	return fmt::format("{}/{}.txt", this->GetPageGroupStoragePath(pg), ghobj.hobj.oid.name);
}