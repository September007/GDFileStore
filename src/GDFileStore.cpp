#include"deps.h"
#include"GDFileStore.h"
using  file_object_data_type=GDFileStore::file_object_data_type;
using KVStore_type=GDFileStore::KVStore_type;
#pragma region old
GDFileStore::GDFileStore(const string& StorePath,const string &fsName) :
	path(std::filesystem::absolute(StorePath).generic_string()),
	fsname(fsName),
	kv(),
	journal(),
	timercaller(0),
	tp(thread::hardware_concurrency()),
	handle_wope_thread(&GDFileStore::handle_wope_threadMain,this)
{
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	//set submodules root
	kv.SetPath( GetKVRoot());
	journal.SetPath( GetJournalRoot());
}
GDFileStore::~GDFileStore() { 
	timercaller.shutdown();
	_shut_handle_wope_thread();
	auto p = this;
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
#pragma endregion old
//read from kv,this should only be call if there is one
//ObjectWithRB GDFileStore::GetObjectWithRBForGHObj(const GHObject_t& ghobj) {
//	auto gh = as_string(ghobj);
//	string rb;
//	kv.GetValue(fmt::format("gh_to_rb::{}", gh), rb);
//	return from_string<ObjectWithRB>(rb);
//}

//ObjectWithRB GDFileStore::CreateObjectWithRBForGHObj(const GHObject_t& ghobj) {
//	auto ret = ReferedBlock::getNewReferedBlock();
//	//bind this to kv;
//	auto gh= as_string(ghobj);
//	auto rb = as_string(ret);
//	auto r1=kv.SetValue(fmt::format("rb_to_gh::{}", rb), gh);
//	auto r2=kv.SetValue(fmt::format("gh_to_rb::{}", gh), rb);
//	LOG_EXPECT_TRUE("projection", r1.ok());
//	LOG_EXPECT_TRUE("projection", r2.ok());
//	return ret;
//}

//string GDFileStore::ReadGHObj(const GHObject_t& ghoj) {
//	auto rb = GetObjectWithRBForGHObj(ghoj);
//	auto rd = ReadReferedBlock(rb, GetReferedBlockRootPath());
//
//}

void GDFileStore::handle_wope_threadMain() {
	auto pthis = this;
	while (!shut_handle_wope_thread) {
		list<WOpeLog> tmp;
		{
			unique_lock lg(access_to_wope_logs);
			tmp.swap(wope_logs);
		}
		if (tmp.size() == 0)
			this_thread::sleep_for(chrono::milliseconds(1));
		else {
			for (auto& wope : tmp) {
				pthis->tp.enqueue([pthis = pthis, wo = move(wope)]{
					pthis->do_wope(wo);
					});
			}
		}
	}
}
//@dataflow wope_log add to task list
void GDFileStore::add_wope(WOpeLog wopelog) {
	unique_lock lg(access_to_wope_logs);
	wope_logs.push_back(move(wopelog));
}

//@dataflow wope_log do it
void GDFileStore::do_wope(WOpeLog wopelog) {
	auto& wope = wopelog.wope;
	switch (wopelog.wope_state) {
		case WOpeLog::wope_state_Type::init:
		{//write journal
			journal.do_WOPE(wopelog);
			//@dadaflow reqWrite::journalWrite
			buffer buf;
			auto from = this->GetInfo();
			auto rt = repType::primaryJournalWrite;
			MultiWrite(buf, from, rt, wopelog.opeId);
			//@dataflow request repWrite
			http_send(this->GetInfo(), wopelog.from, buf, "/repWrite");
			//@dataflow wopelog change init to obJournal
			wopelog.wope_state = WOpeLog::wope_state_Type::onJournal;
			add_wope(move(wopelog));
		}break;
		case WOpeLog::wope_state_Type::onJournal:
		{//move journal data to disk
			//move new referedblocks to disk root
			//how to define new?,when these rb record as refer_count==1 mean new
			auto ngh = wope.new_ghobj;
			auto porb = kv.GetAttr(ngh);
			for (auto& rbs : porb.first.serials_list) {
				auto rb = ReferedBlock(rbs);
				auto rb_attr = kv.GetAttr(rb).first;
				if (rb_attr.refer_count == 1) {
					auto from_path = GetReferedBlockStoragePath(rb_attr, GetJournalRBRoot());
					auto to_path= GetReferedBlockStoragePath(rb_attr, GetRBRoot());
					filesystem::copy(from_path, to_path);
				}
			}
			wopelog.wope_state = WOpeLog::wope_state_Type::onDisk;
			add_wope(move(wopelog));
		}break;
		case WOpeLog::wope_state_Type::onDisk:
		{//@dataflow repWrite pack 
			buffer buf;
			auto from = this->GetInfo();
			auto rt = repType::primaryDiskWrite;
			MultiWrite(buf, from, rt, wopelog.opeId);
			http_send(this->GetInfo(), wopelog.from, move(buf), "/repWrite");
		}break;
		default:
			break;
	};
}

ReferedBlock GDFileStore::addNewReferedBlock(string data,string root_path) {
	auto rb = ReferedBlock::getNewReferedBlock();
	rb.refer_count = 0;
	WriteReferedBlock(rb, root_path, data);
	return rb;
}

ROPE_Result GDFileStore::ReadGHObj(const GHObject_t& ghoj, ROPE rope) {
	return ROPE_Result();
}

void GDFileStore::handleReadGHObj(const GHObject_t& ghoj, InfoForNetNode from) {
}

void GDFileStore::handleModifyGHObj(const GHObject_t& ghobj, const GHObject_t& new_ghobj, WOPE wope, InfoForNetNode from, vector<InfoForNetNode> tos, reqType rt) {
}
