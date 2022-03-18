#include<test_head.h>
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
	GHObject_t ghobj(HObject_t(Object_t("test_object")));
	auto buf = make_shared<buffer>();
	auto s0 = GetSliceFromWriteSequence(buf, "12345", 5);
	auto s1 = GetSliceFromWriteSequence(buf, "67890", 5);
	auto s2 = Slice(nullptr, 0, 4);
	auto s3 = GetSliceFromWriteSequence(buf, "abcd", 4);

	auto o0 = Operation(OperationType::Insert, s0, FilePos(0, FileAnchor::begin));//insert 12345 at 0, 12345
	auto o1 = Operation(OperationType::Insert, s1, FilePos(5, FileAnchor::begin));//insert 67890 at 5,1234567890
	auto o2 = Operation(OperationType::Delete, s2, FilePos(3, FileAnchor::begin));//delete span[4] at 3, 123890
	auto o3 = Operation(OperationType::Insert, s3, FilePos(3, FileAnchor::begin));//insert abcd at 3, 123abcd890
	vector<Operation> operations = { o0,o1,o2,o3 };
	string expect_buffer = "123abcd890";

	auto testOPs = [](const vector<Operation>& ops, const string& expect) {
		auto objData = make_shared<buffer>();
		int indicated_length = 0;
		auto slices = Operation::CombineOperationsForOneSameObject(objData, ops, indicated_length);
		auto finalBuffer = Operation::GetBufferFromSlices(slices);
		string finalBufferstr(finalBuffer->data, finalBuffer->data + finalBuffer->length);
		EXPECT_EQ(expect, finalBufferstr);
	};
		// BufferFrom and Operation Wrapper
	testOPs(operations, expect_buffer);
	auto oo1 = OperationWrapper::WriteWrapper(ghobj, "12345");
	auto oo2 = OperationWrapper::AppendWrapper(ghobj, "67890");
	auto oo3 = OperationWrapper::DeleteWrapper(ghobj, 3, 7);
	auto oo4 = OperationWrapper::InsertWrapper(ghobj, "abcd", 3);
	auto oops = vector<Operation>{ oo1,oo2,oo3,oo4 };
	testOPs(oops, expect_buffer);

}
