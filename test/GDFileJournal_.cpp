#include<FileJournal.h>
#include<gtest/gtest.h>
#define head FileJournal

auto GetSliceFromWriteSequence(shared_ptr<buffer>buf, const char* t, int size) {
	auto ret = Slice(buf, buf->length, 0);
	WriteSequence(*buf.get(), t, size);
	ret.end = buf->length;
	return ret;
}
//test writes combinition
TEST(head, CombineOperationsForOneSameObject) {
	auto buf = make_shared<buffer>();
	auto s0 = GetSliceFromWriteSequence(buf, "12345", 5);
	auto s1 = GetSliceFromWriteSequence(buf, "67890", 5);
	auto s2 = Slice(nullptr, 0, 4);
	auto s3 = GetSliceFromWriteSequence(buf, "abcd", 4);

	auto o0 = Operation(OperationType::Insert, s0, FilePos(0, FileAnchor::begin));//insert 12345 at 0, 12345
	auto o1 = Operation(OperationType::Insert, s1, FilePos(5, FileAnchor::begin));//insert 67890 at 5,1234567890
	auto o2 = Operation(OperationType::Delete, s2, FilePos(3, FileAnchor::begin));//delete span[4] at 3, 123890
	auto o3 = Operation(OperationType::Insert, s3, FilePos(3, FileAnchor::begin));//insert abcd at3, 123abcd899
	vector<Operation> operations = { o0,o1,o2,o3 };
	auto objData = make_shared<buffer>();
	int indicated_length = 0;
	auto slices = Operation::CombineOperationsForOneSameObject(objData, operations, indicated_length);
	auto finalBuffer = Operation::GetBufferFromSlices(slices);
	string expect_buffer = "123abcd890";
	string finalBufferstr(finalBuffer->data, finalBuffer->data + finalBuffer->length);
	EXPECT_EQ(expect_buffer, finalBufferstr);
	int *das= new int[10];
	das[100] = 231231;
	int iv100[100];
	ivc100[1000] = 1;
}