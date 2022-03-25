#pragma once
#include <assistant_utility.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <vector>
#include<deps/deps.h>
#include<GDKeyValue.h>
using namespace std;
class Journal {
public:
	Journal(const string& storagePath = "") { SetPath(storagePath); }
	//@Todo bloom filter for current journal found
	void Init(const string& path, RocksKV* kv);
	void SetPath(const string& path);
	void SetKV(RocksKV* kv);
	auto Write(const string& objDirHashInt, const string& data) {
		return WriteFile(objDirHashInt, data);
	}
	auto Read(const string& objDirHashInt) { 
		return ReadFile(objDirHashInt);
	}
	/*
	* handle Write operation
	*/
	void HandleWriteOperation(const Operation& wOpe, bool emengency=false);

	void HandleWriteOperation(const vector<Operation>& wOpe, bool emengency = false);

public:
	string storagePath;
	// registed write 
	vector<Operation> wOpes;
	// just refer, doesn't obligated to free this
	RocksKV* kv;

private:
	//combine and write
	void _flushProcessor();
	// may need mutex access wOpes
	inline void _AddWriteOperation(const Operation& wOpe);
	inline void _AddWriteOperation(const vector<Operation>& wOpe);
};