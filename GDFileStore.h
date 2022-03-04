#include<deps.h>
//@Todo: journal
class Object {
	string name;
};
//@Todo: kv store
class KVStore {

};
class GDFileStore {
//types define
public:
	using KVStore_type = KVStore;
	using file_object_data_type = std::string_view;

// private member data
private:
	//objects attributes data
	KVStore_type kv;
	//local filesystem store path for objects data
	string path;

public:
	template<typename ...KVInitialArgs>
	GDFileStore(const string& StorePath, KVInitialArgs&&... kvinitialArgs) :
 		path(StorePath), kv(std::forward<KVInitialArgs&&>( kvinitialArgs)...) {};
	GDFileStore(const GDFileStore&) = delete;
	
	//attributes interface for GET 

	auto get_KVStore() { return &kv; }
	auto get_path() { return 1; }

	//@Todo: transaction
	//@Todo: ops
	//@Todo: mount unmount

};