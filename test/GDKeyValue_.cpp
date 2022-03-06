#include<gtest/gtest.h>
#include<GDKeyValue.h>
#include<filesystem>
#include<assistant_utility.h>
#include<memory>
using namespace std;

auto path = filesystem::absolute("./tmp/testdb").string();
constexpr auto tries = 1000;
auto createDB(const string& path) {
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	rocksdb::DB* db;
	rocksdb::Options options;
	options.create_if_missing = true;
	rocksdb::Status status =
		rocksdb::DB::Open(options, path, &db);
	EXPECT_TRUE(status.ok());
	return db;
}
TEST(rocksdb, put_get_delete) {
	//create && delete
	auto db=createDB(path);
	auto rp = rocksdb::ReadOptions();
	auto wp = rocksdb::WriteOptions();
	vector<string> ks(tries), vs(tries);
	randomValue(&ks[0], tries);
	randomValue(&vs[0], tries);
	//store
	for (int i = 0; i < tries; ++i) {
		db->Put(wp, ks[i], vs[i]);
	}
	//get
	for (int i = 0; i < tries; ++i) {
		string value;
		auto status = db->Get(rp, ks[i], &value);
		EXPECT_EQ(value, vs[i]);
	}
	//delete
	for (int i = 0; i < tries; ++i) {
		string value;
		auto status = db->Delete(wp, ks[i]);
		EXPECT_TRUE(status.ok());
		status = db->Get(rp, ks[i], &value);
		EXPECT_EQ(value, "");
		EXPECT_TRUE(!status.ok());
	}
	
	delete db;
	filesystem::remove_all(path);
}