#pragma once
#include<string>
#include<nlohmann/json.hpp>
#include<assistant_utility.h>
#include<memory>
using namespace std;

class BufferRef {
public:
	shared_ptr<buffer>buf;
	BufferRef(buffer* buf) :buf(buf) {};
};
enum  class OprationType
{
	Insert,
	Delete,
	Replace
};
class WriteOp {
};

class Journal {
public:
	string storagePath;
	Journal(const string & storagePath="") :storagePath(storagePath) {}
	//@Todo bloom filter for current journal found

	auto Write(const string& objDirHashInt, const string& data) {
		return WriteFile(objDirHashInt, data);
	}
	auto Read(const string& objDirHashInt) {
		return ReadFile(objDirHashInt);
	}
};