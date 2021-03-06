#include<object.h>


string GHObject_t::GetLiteralDescription(GHObject_t const& ghobj) {
	return fmt::format("{}.{}", ghobj.owner, ghobj.hobj.oid.name);
}

vector<Slice> Operation::CombineOperationsForOneSameObject
(const vector<Operation>& operations, int& out_totalLength) {
	//@Todo: interval tree
	//@Todo: at least rewrite it usr list
	vector<Slice> list;
	// length of whole object data for now
	auto totalLength = 0;
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
				auto deleteBegin = filePos + operation.data.start;
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

vector<Slice> Operation::CombineOperationsForOneSameObject(shared_ptr<buffer> objData, const vector<Operation>& operations, int& out_totalLength) {
	//objData->data.length();
	//(objData, 0, objData->data.length());
	auto originData = OperationWrapper::WriteWrapper(operations[0].obj, objData->universal_str());
	auto new_operations = vector<Operation>{ originData };
	new_operations.insert(new_operations.end(), operations.begin(), operations.end());
	return CombineOperationsForOneSameObject(new_operations, out_totalLength);
}

shared_ptr<buffer> Operation::GetBufferFromSlices(const vector<Slice>& slices, int indicated_length) {
	auto buf = make_shared<buffer>();
	for (auto slice : slices) {
		LOG_EXPECT_TRUE("integrated", slice.data != nullptr);
		if (slice.data != nullptr)
			WriteSequence(*buf.get(), slice.data->data.c_str() + slice.start, slice.GetSize());
	}
	return buf;
}
int Operation::GetFilePos(const Operation ope, int endPos) {
	if (ope.filePos.fileAnchor == FileAnchor::begin)
		return ope.filePos.offset;
	else
		return ope.filePos.offset + endPos;
}
