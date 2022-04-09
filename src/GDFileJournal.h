#pragma once
#include <assistant_utility.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <vector>
#include<deps/deps.h>
#include<GDKeyValue.h>
#include<forward_declare.h>
using namespace std;
class Journal {
public:
	Journal(const string& storagePath = "") { SetPath(storagePath); }
	//@Todo bloom filter for current journal found
	//note: storagePath need to be set before
	//need kv to load info
	bool Mount(RocksKV* kv, GDFileStore* fs);
	bool UnMount();
	void SetPath(const string& path);
	void SetKV(RocksKV* kv);
	void SetFS(GDFileStore* kv);
	auto Write(const string& objDirHashInt, const string& data) {
		return WriteFile(fmt::format("{}/{}",storagePath,objDirHashInt), data);
	}
	//@dataflow objdata journal_read
	//if kv have journal record, read it otherwise read filestore file
	pair<bool, string> ReadObject(const GHObject_t& gh);
	auto Read(const string& objDirHashInt) { 
		return ReadFile(fmt::format("{}/{}", storagePath, objDirHashInt));
	}
	/*
	* handle Write operation
	*/
	void HandleWriteOperation(const Operation& wOpe, bool emengency=false);

	void HandleWriteOperation(const vector<Operation>& wOpe, bool emengency = false);

public:
	string storagePath;
	string log_journal_head;
	// registed write 
	vector<Operation> wOpes;
	mutex access_to_wOpes;
	// just refer, doesn't obligated to free this
	RocksKV* kv;
	GDFileStore* fs;
private:
	//combine and write
	void _flushProcessor();
	// may need mutex access wOpes
	inline void _AddWriteOperation(const Operation& wOpe);
	inline void _AddWriteOperation(const vector<Operation>& wOpe);



public:
	void do_WOPE(WOpeLog wopelog);

};