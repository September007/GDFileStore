#pragma once
#ifndef GDFILESTORE_HEAD
#define GDFILESTORE_HEAD
#include<deps.h>
#include<sys/stat.h>
#include<fmt/format.h>
#include<fstream>
#include<filesystem>
#include<spdlog/spdlog.h>
#include<GDKeyValue.h>
#include<deps/object.h>
#include<FileJournal.h>
//@Todo: journal

//for PageGroup and Object,the default value of them  means those are meaningless or illegal
template<typename T>
	requires is_default_constructible_v<decay_t<T>>
inline bool is_default(T &&t) { return t == decay_t<T>(); };

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

public:
	
	explicit GDFileStore(const string& StorePath) :
		path(std::filesystem::absolute(StorePath).generic_string()),
		kv(fmt::format("{}/kv", std::filesystem::absolute(StorePath).generic_string())) ,
		journal(fmt::format("{}/journal", std::filesystem::absolute(StorePath).generic_string())){
			if(!filesystem::is_directory(path))
			filesystem::create_directory(path);
	};
	GDFileStore(const GDFileStore&) = delete;
	GDFileStore(GDFileStore&&) = delete;
	//attributes interface for GET 

	auto get_KVStore() { return &kv; }
	auto get_path() { return path; }
	//load kv,return true if success
	auto Mount() {
		if (!kv.LoadDB()) {
			GetLogger("GDFileStore")->error("load kv db failed when mount[{}].{}:{}", this->path, __FILE__, __LINE__);
			return false;
		}
		return true;
	}
	auto UnMount() {
		return kv.UnLoadDB();
	}
	//journal
	Journal journal;

private: 
	//internal interface for object data
	//file suffix is .txt
	auto _WriteJournal(GHObject_t obj, const string& content) {
		auto objDirHashInt = to_string(std::hash<string>()(obj.hobj.oid.name));
		auto ret= journal.Write(objDirHashInt, content);
		if (!ret) {
			GetLogger("Write")->error("write journal {} failed.{}:{}", objDirHashInt, __FILE__, __LINE__);
			return false;
		}
		//value set empty
		kv.SetValue("journal::write_" + _GetObjectStoragePath(obj), objDirHashInt);
	}
	auto flushWriteJournal() {
		auto allWriteJournal = kv.GetMatchPrefix("journal::write_");
		for (auto& writeJournal : allWriteJournal) {
			auto objDir = writeJournal.first. substr(string("journal::write_").size());
			auto objContent = journal.Read(writeJournal.second);
			WriteFile(objDir, objContent);
		}
	}
	string _GetObjectStoragePath(GHObject_t obj) {
		auto pg = _GetPageGroup(obj);
		return fmt::format("{}/{}/{}.txt", this->path, pg.name, obj.hobj.oid.name);
	}
	string _GetPageGroupStoragePath(PageGroup pg) {
		return fmt::format("{}/{}", this->path, pg.name);
	}
	bool _IsDirectoryExists(const char* path) {
		return filesystem::is_directory(path);
	}
	bool _IsFileExists(const char* path) {
		return filesystem::is_regular_file(path);
	}
	bool _IsObjectExists(GHObject_t obj) {
		return _IsFileExists(obj.hobj.oid.name.c_str());
	}
	bool _CreateDir(const char* path) {
		return std::filesystem::create_directories(path);
	}
	//call this after confirm object exists
	file_object_data_type _get_object_data(GHObject_t obj) {
		return ReadFile(_GetObjectStoragePath(obj));
	}
	//return true if success
	//@Todo: save file in binary if content contains illegal char
	//@Todo: postphone to mannage coruntine to improving performance and potential lack of descriptor
	bool _StoreObjectData(GHObject_t obj, file_object_data_type data) {
		auto objDir = _GetObjectStoragePath(obj);
		return 	WriteFile(objDir, data, true);
	}
	//@Todo: transaction
	//@Todo: ops
public:
	/*
	 * the projection from Object to PageGroup, here just  defining as a simple linear mod after hash
	 * in the future ,this could become a search in a metadata map
	 */
	PageGroup _GetPageGroup(const GHObject_t obj) {
		constexpr int PG_count_limit = 10;
		if (is_default(obj))return PageGroup();
		return PageGroup(to_string(std::hash<string>()(obj.hobj.oid.name)%PG_count_limit));
	}
};

#endif //GDFILESTORE_HEAD