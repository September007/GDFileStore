#include"deps.h"
#include"GDFileStore.h"
using  file_object_data_type=GDFileStore::file_object_data_type;
using KVStore_type=GDFileStore::KVStore_type;
file_object_data_type GDFileStore::_get_object_data(Object obj)
{
	fstream in(_GetObjectStoragePath(obj));
	in.seekg(0, ios_base::end);
	size_t length = in.tellg();
	in.seekg(ios_base::beg);
	string ret;
	//the last char on file[length-1] is '\0'
	ret.resize(length);
	in.read(const_cast<char*>(ret.c_str()), length);
	if (ret.back() == '\0')ret.pop_back();
	return ret;
}

bool GDFileStore::_StoreObjectData(Object obj, file_object_data_type data)
{
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