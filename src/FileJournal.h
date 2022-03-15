#pragma once
#include <assistant_utility.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <vector>
#include<deps/deps.h>
using namespace std;
// basic operations only have the below two
enum class OperationType {
	Insert,
	Delete
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
	Object_t obj;
	// operation begin point
	FilePos filePos;
	Operation(OperationType operationType, Slice data, FilePos filePos)
		: operationType(operationType)
		, data(data)
		, filePos(filePos) {
	}
	// combine writes on the same object
	static vector<Slice> CombineOperationsForOneSameObject(shared_ptr<buffer> objData,
		const vector<Operation>& operations,int &out_totalLength) {
		//@Todo: interval tree
		//@Todo: at least rewrite it usr list
		vector<Slice> list;
		// length of whole object data for now
		auto totalLength = objData->length;
		list.emplace_back(objData, 0, objData->length);
		for (auto& operation : operations) {
			auto filePos = GetFilePos(operation, totalLength);
			LOG_EXPECT_TRUE("Journal", filePos >= 0 && filePos <= totalLength);
			switch (operation.operationType) {
				case OperationType::Insert:
				{
					//update list
					if (filePos == totalLength) {
						// insert at the end
						list.push_back(operation.data);
					}
					else {
						int i = 0;
						for (; i < list.size(); i++) {
							//note if current slice.size()==0,keep forward
							if (filePos >= list[i].GetSize()) {
								filePos -= list[i].GetSize();
								continue;
							}
							else break;
						}
						//here i must satisfy i<list.size() ,because we specially split the {insert at the end }at above
						LOG_EXPECT_TRUE("Journal", i < list.size());
						if (filePos == 0) {
							//insert at list[i]
							list.insert(list.begin() + i, operation.data);
						}
						else {
							//split apart
							Slice firstPart = list[i].SubSlice(0, filePos);//(list[i].data, list[i].start, filePos + list[i].start);
							Slice newSlices[] = {
								operation.data,
								list[i].SubSlice(filePos,list[i].GetSize() - filePos)
							};
							list[i] = firstPart;
							list.insert(list.begin() + i + 1, newSlices, newSlices + 2);
						}
					}
					//update length
					totalLength += operation.data.GetSize();
					break;
				}

				case OperationType::Delete:
				{
					//check @contract_1
					LOG_EXPECT_EQ("Journal", operation.data.start, 0);
					//check @contract_2
					LOG_EXPECT_TRUE("Journal", operation.data.GetSize() + filePos <= totalLength);
					auto deleteBegin = filePos;
					auto deleteSpan = operation.data.GetSize();
					int i = 0;
					//lowebound
					for (; i < list.size(); ++i) {
						if (deleteBegin >= list[i].GetSize())
							deleteBegin -= list[i].GetSize();
						else
							break;
					}
					Slice ForeRest = list[i].SubSlice(0, deleteBegin);
					if (deleteBegin + deleteSpan <= list[i].GetSize()) {
						//if delete range is  in one slice ,then insert a new slice
						Slice BackRest = list[i].SubSlice(deleteBegin + deleteSpan, list[i].GetSize() - deleteBegin - deleteSpan);
						list[i] = ForeRest;
						list.insert(list.begin() + i + 1, BackRest);
					}
					else {
						int j = i;
						auto deleteEnd = deleteBegin + deleteSpan;
						for (; j < list.size(); ++j) {
							if (deleteEnd >= list[j].GetSize()) {
								deleteEnd -= list[j].GetSize();
							}
							else break;
						}
						list[i] = ForeRest;
						list[j] = list[j].SubSlice(deleteEnd, list[j].GetSize() - deleteEnd);
						list.erase(list.begin() + i + 1, list.begin() + j);
					}
					totalLength -= operation.data.GetSize();
					break;
				}
				default:
					LOG_EXPECT_TRUE("Journal", false);
			}
		}
		out_totalLength = totalLength;
		return list;
	}
	static shared_ptr<buffer> GetBufferFromSlices(vector<Slice> slices, int indicated_length = 0) {
		auto buf = make_shared<buffer>();
		for (auto slice : slices) {
			WriteSequence(*buf.get(), slice.data->data + slice.start, slice.GetSize());
		}
		return buf;
	}
	static int GetFilePos(const Operation ope, int endPos) {
		if (ope.filePos.fileAnchor == FileAnchor::begin)
			return ope.filePos.offset;
		else
			return ope.filePos.offset + endPos;
	}
};
class Journal {
public:
	string storagePath;
	Journal(const string& storagePath = "")
		: storagePath(storagePath) {
	}
	//@Todo bloom filter for current journal found

	auto Write(const string& objDirHashInt, const string& data) {
		return WriteFile(objDirHashInt, data);
	}
	auto Read(const string& objDirHashInt) { return ReadFile(objDirHashInt); }
};