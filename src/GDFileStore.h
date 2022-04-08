#pragma once
#ifndef GDFILESTORE_HEAD
#define GDFILESTORE_HEAD
#include<forward_declare.h>
#include<fstream>
#include<filesystem>
#include<fmt/format.h>
#include<spdlog/spdlog.h>
#include<httplib.h>
#include<deps.h>
#include<deps/object.h>
#include<GDKeyValue.h>
#include<GDFileJournal.h>
#include<deps/connection.h>
#include<projection.h>
#include<timer.h>
#include<referedBlock.h>
#pragma region old
//for PageGroup and Object,the default value of them  means those are meaningless or illegal
template<typename T>
	requires is_default_constructible_v<decay_t<T>>
inline bool is_default(T&& t) { return t == decay_t<T>(); };

//1. call LoadDB() before use db
//2. 
class GDFileStore {
#ifdef TEST_ON
	//this is test class,which need access
	friend struct GDFSTest;
#endif
	//types define
public:
	using KVStore_type = RocksKV;
	using file_object_data_type = std::string;

	// private member data
private:
	//objects attributes data
	KVStore_type kv;
	//local filesystem store path for objects data
	string path;
	//journal
	Journal journal;
	ConcTimerCaller timercaller;
public:
	//@new , count on this to get a specified configuration from  a bunch of files at [/src/config]
	string fsname;// = "default_fs";

public:

	explicit GDFileStore(const string& StorePath,const string &fsName="default_fs");
	GDFileStore(const GDFileStore&) = delete;
	GDFileStore(GDFileStore&&) = delete;
	~GDFileStore();

	auto get_KVStore() { return &kv; }
	auto get_path() { return path; }
	//load kv,return true if success
	auto Mount() {
		if (!kv.LoadDB()) {
			GetLogger("GDFileStore")->error("load kv db failed when mount[{}].{}:{}", this->path, __FILE__, __LINE__);
			return false;
		}
		if (!journal.Mount(&kv,this)) {
			GetLogger("GDFileStore")->error("load journal failed when mount[{}].{}:{}", this->path, __FILE__, __LINE__);
			return false;
		}
		auto conc = GetconfigOverWrite(4, "FileStore", GetOsdName(), "concurrency");
		timercaller.tp.active(conc);
		return true;
	}
	auto UnMount() {
		if(!kv.UnLoadDB()||!journal.UnMount())
		{
			LOG_ERROR("GDFileStore",fmt::format("unmount failed"));
			return false;
		}
		return true;
	}
	//@new 22-3-25
	bool HandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds);
	// obligated to call followers 
	bool PrimaryHandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds);
	//just do own job
	bool ReplicaHandleWriteOperation(const Operation& wope, const vector<InfoForOSD>& osds);
	//@dataflow journal flush filestore will flush journal when timer call
	//@mutex journal::access_to_wOpes 
	//@emergency thread_pool timer to call this every interval
	void flushJouranl();
private:
	//internal interface for object data
	//file suffix is .txt
	auto _WriteJournal(GHObject_t obj, const string& content);
	auto flushWriteJournal();
	bool _IsDirectoryExists(const string& path) {
		return filesystem::is_directory(path);
	}
	bool _IsFileExists(const string& path) {
		return filesystem::is_regular_file(path);
	}
	//@Todo : bloom filter
	bool _IsObjectExists(GHObject_t obj) {
		return _IsFileExists(GetGHObjectStoragePath(obj));
	}
	bool _CreateDir(const char* path) {
		return std::filesystem::create_directories(path);
	}
	//call this after confirm object exists
	file_object_data_type _get_object_data(GHObject_t obj) {
		auto objDir = GetGHObjectStoragePath(obj);
		return ReadFile(objDir);
	}
	//return true if success
	//@Todo: save file in binary if content contains illegal char
	//@Todo: postphone to mannage coruntine to improving performance and potential lack of descriptor
	bool _StoreObjectData(GHObject_t obj, file_object_data_type data) {
		auto objDir = GetGHObjectStoragePath(obj);
		return 	WriteFile(objDir, data, true);
	}
	//@Todo: transaction
public:
	/*
	 * the projection from Object to PageGroup, here just  defining as a simple linear mod after hash
	 * in the future ,this could become a search in a metadata map
	 */
	 // projection from hobj(already have pool infomation) to pg
	inline PageGroup GetPageGroupFroGHOBJ(const GHObject_t& ghobj)const;
	/*
	* Storage Path Management
	*/
	//kv root
	string GetKVRoot()const {
		return fmt::format("{}/KV", this->path);
	}
	//journal root
	string GetJournalRoot()const {
		return fmt::format("{}/Journal", this->path);
	}
	//pgs parent path
	string GetPageGroupsRoot()const {
		return fmt::format("{}/PG", this->path);
	}
	//pg path
	string GetPageGroupStoragePath(const PageGroup& pg) const {
		return fmt::format("{}/{}", this->GetPageGroupsRoot(), pg.name);
	}
	//obj path
	string GetGHObjectStoragePath(const GHObject_t& ghobj) const;
	
	string GetOsdName() { return this->fsname; };

	InfoForOSD GetInfo() { return GetOSDConfig(GetOsdName()).second; }




#pragma endregion 
	//@new for block storage
	
	GD::ThreadPool tp;
	//refered blocks path root
	string GetReferedBlockRootPath() {
		return fmt::format("{}/rb", this->path);
	}
	//read

	//ObjectWithRB GetObjectWithRBForGHObj(const GHObject_t& ghobj);
	//ObjectWithRB CreateObjectWithRBForGHObj(const GHObject_t& ghobj);

//	string ReadGHObj(const GHObject_t& ghoj);
	ROPE_Result ReadGHObj(const GHObject_t& ghoj, ROPE rope);

	//handle request
	void handleReadGHObj(const GHObject_t& ghoj,InfoForNetNode from);
	void handleModifyGHObj(const GHObject_t& ghobj, const GHObject_t& new_ghobj,
		WOPE wope, InfoForNetNode from,vector<InfoForNetNode> tos, reqType rt);
	
};

#endif //GDFILESTORE_HEAD