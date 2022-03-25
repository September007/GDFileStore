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
// basic operations only have the below two
enum class OperationType {
	Insert,
	Delete
};
// operation state  for detail see file design.vsdx
enum class OperationState {
	//for write
	wrapped,inJournalQueue,inJournalCache,inFS
	//for read
};
// file anchor like std::fstream::begin
enum class FileAnchor {
	begin = 1,
	end = 2
};
// point out a point position
class FilePos {
public:
	FileAnchor fileAnchor;
	int offset;
	FilePos(int offset, FileAnchor fileAnchor = FileAnchor::begin)
		: offset(offset)
		, fileAnchor(fileAnchor) {
	}
	FilePos(const FilePos&) = default;
	bool operator==(const FilePos&) const = default;
};
class Operation {
public:
	// operation type, insert or delete
	OperationType operationType;
	/* operation data
	* when in delete, only its start and end matter ,and the span of being delete is end-start(as Slice.GetSize())
	* @contract_1  start==0,see LOG_EXEPECT below
	* @contract_2 data.size()+filePos<=objLength
	*/
	Slice data;
	// operation related object
	GHObject_t obj;
	// operation begin point
	FilePos filePos;
	Operation(const GHObject_t &ghobj,OperationType operationType, Slice data, FilePos filePos)
		: obj(ghobj)
		, operationType(operationType)
		, data(data)
		, filePos(filePos) {
	}
	// combine writes on the same object
	static vector<Slice> CombineOperationsForOneSameObject(const vector<Operation>& operations,
		int& out_totalLength);
	static vector<Slice> CombineOperationsForOneSameObject(shared_ptr<buffer> objData,
		const vector<Operation>& operations, int& out_totalLength);
	static shared_ptr<buffer> GetBufferFromSlices(const vector<Slice>& slices, int indicated_length = 0);
	static int GetFilePos(const Operation ope, int endPos);

};

class OperationWrapper {
public:
	// overwrite
	static Operation WriteWrapper(GHObject_t const& ghobj, const string& data) {
		return Operation(ghobj , OperationType::Insert, Slice(data), FilePos(0, FileAnchor::begin));
	}
	// insert a string
	static Operation InsertWrapper(GHObject_t const& ghobj, const string& data, int pos) {
		return Operation(ghobj, OperationType::Insert, Slice(data), FilePos(pos, FileAnchor::begin));
	}
	// delete all
	static Operation DeleteWrapper(GHObject_t const& ghobj, int SpanBegin, int SpanEnd) {
		return Operation(ghobj, OperationType::Delete, Slice(nullptr, SpanBegin, SpanEnd), FilePos(0, FileAnchor::begin));
	}
	// delete span
	//static Operation DeleteWrapper(GHObject_t const& ghobj, int SpanBegin, int SpanEnd) {
	//	return Operation(OperationType::Delete, Slice(nullptr, SpanBegin, SpanEnd), FilePos(0, FileAnchor::begin));
	//}
	// append at end
	static Operation AppendWrapper(GHObject_t const& ghobj, const string& data) {
		return Operation(ghobj, OperationType::Insert, Slice(data), FilePos(0, FileAnchor::end));
	}
};
//@Todo.t2 use this!!
// each operation have a callback
using OperationCallBackType = std::function<void(Operation *)>;
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
	void WriteOperation(const Operation& wOpe);
	void WriteOperation(const vector<Operation>& wOpe);
	//combine and write
	void _flushProcessor();
	// may need mutex access wOpes
	inline void _AddWriteOperation(const Operation& wOpe);
	inline void _AddWriteOperation(const vector<Operation>& wOpe);
};