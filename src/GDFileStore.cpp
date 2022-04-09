#include"deps.h"
#include"GDFileStore.h"
using  file_object_data_type=GDFileStore::file_object_data_type;
using KVStore_type=GDFileStore::KVStore_type;
inline auto GetParentStr(string& path) {
	DebugArea(auto abpath = filesystem::absolute(path); LOG_EXPECT_EQ("IO", abpath.string(), path));
	for (int i = path.size() - 1; i >= 0; --i)
		if (path[i] != '/' && path[i] != '\\')
			path.pop_back();
		else break;
	return path;
}
#pragma region old
GDFileStore::GDFileStore(const string& StorePath, const string& fsName) :
	path(std::filesystem::absolute(StorePath).generic_string()),
	fsname(fsName),
	kv(),
	journal(),
	timercaller(thread::hardware_concurrency()),
	universal_tp(3),//(thread::hardware_concurrency()),
	write_tp(thread::hardware_concurrency()),
	read_tp(thread::hardware_concurrency()),
	handle_ope_thread()
{
	journal.SetPath(GetJournalRoot());
	kv.SetPath(GetKVRoot());
	LOG_EXPECT_TRUE("filestore", Mount());
	{
		thread t(&GDFileStore::handle_ope_threadMain, this);
		handle_ope_thread.swap(t);
	}
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	auto conc = GetconfigOverWrite(8, "FileStore", GetOsdName(), "concurrency");
	timercaller.tp.active(conc);
	//timercaller run in universal_tp
	universal_tp.enqueue([pthis = this] {pthis->timercaller.run(); });
	LOG_EXPECT_TRUE("filestore", Mount());
}
GDFileStore::~GDFileStore() { 
	//shutdown threadpools,even them can be shutdown in deconstruct
	//shut timercaller first,cuz timercaller is run in univeral_tp
	timercaller.shutdown();
	universal_tp.shutdown();
	write_tp.shutdown();
	read_tp.shutdown();
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

void GDFileStore::handle_ope_threadMain() {
	auto pthis = this;
	while (!shut_handle_ope_thread) {
		list<WOpeLog> wtmp;
		list<ROpeLog> rtmp;
		{
			unique_lock lg(access_to_wope_logs);
			wtmp.swap(wope_logs);
		}
		if (wtmp.size() == 0)
			;// this_thread::sleep_for(chrono::milliseconds(1));
		else {
			for (auto& wope : wtmp) {
				pthis->write_tp.enqueue([pthis = pthis, wo = move(wope)]{
					pthis->do_wope(wo);
					});
			}
		}
		{
			unique_lock lg(access_to_rope_logs);
			rtmp.swap(rope_logs);
		}
		if (rtmp.size() == 0);
		else {
			for (auto& rope : rtmp)
				pthis->read_tp.enqueue([pthis = pthis, ro = move(rope)]{
					pthis->do_rope(ro);
					});
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
					auto to_parent = GetParentStr(to_path);
					if (!filesystem::exists(to_parent))
						filesystem::create_directories(to_parent);
					filesystem::copy(from_path, to_parent);
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

void GDFileStore::add_rope(ROpeLog ropelog) {
	unique_lock lg(access_to_rope_logs);
	rope_logs.push_back(move(ropelog));
}

void GDFileStore::do_rope(ROpeLog ropelog) {
	//rope
	auto &rope = ropelog.rope;
	auto gh_attr = kv.GetAttr(rope.ghobj).first;
	vector<string> block_datas(rope.blocks.size());
	//load as vector for simpfiying indexing
	vector<decay_t<decltype(gh_attr.serials_list.front())>> serials;
	for (auto s : gh_attr.serials_list)
		serials.push_back(s);
	for (int i = 0; i < rope.blocks.size(); ++i)
		block_datas[i] = ReadReferedBlock(serials[rope.blocks[i]], GetRBRoot());
	ROPE_Result result(ropelog.opeId,ropelog.rope.ghobj, ropelog.rope.blocks, move(block_datas));
	//@dataflow request repRead pack
	buffer buf;
	MultiWrite(buf, result);
	http_send(GetInfo(), ropelog.from, buf, "/repRead");
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
