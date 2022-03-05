#include<deps.h>
#include<sys/stat.h>
#include<boost/format.hpp>
#include<fstream>
#include<filesystem>
#include<spdlog/spdlog.h>
//@Todo: journal

//判断是否是默认值,于PageGroup与Object而言，默认值意味着不是正常对象
template<typename T>
	requires is_default_constructible_v<decay_t<T>>
inline bool is_default(T &&t) { return t == decay_t<T>(); };

class PageGroup {
public:
	string name;
	PageGroup(const string& name="") :name(name) {}
	PageGroup(const string&& name) :name(name) {}
	bool operator==(const PageGroup&)const = default;
	bool operator < (const PageGroup&)const = default;
};

class Object {
public:
	string name;
	Object(const string& name="") :name(name) {}
	Object(const string&& name) :name(name) {}
	bool operator==(const Object&)const = default;
	bool operator < (const Object&)const = default;
};
//@Todo: kv store
class KVStore {

};
class GDFileStore {
//types define
public:
	using KVStore_type = KVStore;
	using file_object_data_type = std::string;

// private member data
private:
	//objects attributes data
	KVStore_type kv;
	//local filesystem store path for objects data
	string path;

public:
	template<typename ...KVInitialArgs>requires is_constructible_v<KVStore_type,KVInitialArgs...>
	explicit GDFileStore(const string& StorePath, KVInitialArgs&&... kvinitialArgs) :
 		path(std::filesystem::absolute(StorePath).generic_string()), kv(std::forward<KVInitialArgs&&>( kvinitialArgs)...) {};
	GDFileStore(const GDFileStore&) = delete;
	GDFileStore(GDFileStore&&) = delete;
	//attributes interface for GET 

	auto get_KVStore() { return &kv; }
	auto get_path() { return path; }

private: //public for now for test
	//internal interface for object data
	friend struct GDFSTest;
	//file suffix is .txt
	string _GetObjectStoragePath(Object obj) {
		auto pg = _GetPageGroup(obj);
		return (boost::format("%1%/%2%/%3%.txt") % this->path % pg.name % obj.name).str();
	}
	string _GetPageGroupStoragePath(PageGroup pg) {
		return (boost::format("%1%/%2%") % this->path % pg.name).str();
	}
	bool _IsDirectoryExists(const char* path) {
		struct _stat buffer;
		return ::_stat(path, &buffer) == 0&&(buffer.st_mode&_S_IFDIR);
	}
	bool _IsFileExists(const char* path) {
		struct _stat buffer;
		return ::_stat(path, &buffer) == 0&&(buffer.st_mode&_S_IFREG);
	}
	bool _IsObjectExists(Object obj) {
		return _IsFileExists(obj.name.c_str());
	}
	bool _CreateDir(const char* path) {
		return std::filesystem::create_directories(path);
	}
	//call this after confirm object exists
	file_object_data_type _get_object_data(Object obj) {
		fstream in(_GetObjectStoragePath(obj));
		in.seekg(0, ios_base::end);
		size_t length = in.tellg();
		in.seekg(ios_base::beg);
		string ret;
		//the last char on file[length-1] is '\0'
		ret.resize(length-1);
		in.read(const_cast<char*>(ret.c_str()), length-1);
		return ret;
	}
	//return true if success
	//@Todo: save file in binary if content contains illegal char
	bool _StoreObjectData(Object obj, file_object_data_type data) {
		auto objDir = _GetObjectStoragePath(obj);
		ofstream out(objDir);
		if (!out.good()) {
			auto pg = _GetPageGroup(obj);
			auto pgDir = _GetPageGroupStoragePath(pg);
			//maybe missing pg directory
			if (!_IsDirectoryExists(pgDir.c_str()) && _CreateDir(pgDir.c_str()))
				out.open(objDir);
			else
				return false;
		}
		out << data << flush;
		out.close();
		return out.good();
	}
	//@Todo: transaction
	//@Todo: ops
	//@Todo: mount unmount
public:
	/*
	 * the projection from Object to PageGroup, here just define as a simple linear mod after hash
	 * in the future ,this could become a search in a map
	 */
	PageGroup _GetPageGroup(const Object obj) {
		constexpr int PG_count_limit = 10;
		if (is_default(obj))return PageGroup();
		
		return PageGroup(to_string(std::hash<string>()(obj.name)%PG_count_limit));
	}
};