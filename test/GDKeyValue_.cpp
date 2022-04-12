#include<test_head.h>
#include<gtest/gtest.h>
#include<GDKeyValue.h>
#include<filesystem>
#include<assistant_utility.h>
#include<memory>
using namespace std;

void remove_replicated_key(vector<string> keys) {
	//remove repliacated key
	set<string> ss;
	for (auto& key : keys) {
		while (!ss.insert(key).second)key = randomValue<string>();
	}
}
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
};
void test_put_get_delete(rocksdb::DB*db) {

	auto error_cnt = 0;
	auto rp = rocksdb::ReadOptions();
	auto wp = rocksdb::WriteOptions();
	vector<string> ks(tries), vs(tries);
	randomValue(&ks[0], tries);
	randomValue(&vs[0], tries);
	remove_replicated_key(ks);
	//store
	for (int i = 0; i < tries; ++i) {
		db->Put(wp, ks[i], vs[i]);
	}
	//get
	for (int i = 0; i < tries; ++i) {
		string value;
		auto vi = vs[i];
		auto status = db->Get(rp, ks[i], &value);
		auto x = value == vi;
		if (!x) {
			error_cnt++;
		}
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

}
TEST(rocksdb, put_get_delete) {
	//create && delete
	auto db = createDB(::path);
	test_put_get_delete(db);
	delete db;
	filesystem::remove_all(path);
};
TEST(RocksKV, interface) {
	RocksKV kv("/tmp/testKV");
	EXPECT_TRUE(kv.LoadDB());
	
	constexpr auto tries = 1000;
	int error_cnt = 0;
	vector<string> keys(tries), values(tries);
	randomValue(&keys[0],tries);
	randomValue(&values[0], tries);
	remove_replicated_key(keys);
	//set
	for (int i = 0; i < tries; ++i)
		kv.SetValue(keys[i], values[i]);
	kv.db->FlushWAL(true);
	//get
	for (int i = 0; i < tries; ++i)
	{
		string got;
		kv.GetValue(keys[i], got);
		auto& vi = values[i];
		auto x = values[i] == got;
		EXPECT_EQ(values[i], got);
		if (!x)
			error_cnt++;
	}
	//remove
	for (int i = 0; i < tries; ++i)
		EXPECT_TRUE(kv.RemoveKey(keys[i]).ok());
	//sets
	vector<pair<RocksKV::key_type, RocksKV::value_type>> kvs(tries);
	for (int i = 0; i < tries; ++i)
		kvs[i] = { keys[i],values[i] };
	kv.SetValues(kvs);
	//gets
	vector<RocksKV::value_type> gets_;
	kv.GetValues(keys, gets_);
	EXPECT_EQ(gets_, values);
	//remove s
	auto ret=	kv.RemoveKeys(keys);
	for (auto r : ret) {
		EXPECT_TRUE(r.ok());
	}
}

TEST(RocksKV, GetWithPrefix) {
	RocksKV kv(filesystem::absolute("/tmp/testdb1").string());
	kv.LoadDB();
	map<string, string> pres = { {"pre","dsd"},
		{"pre0","dsda"},
		{"predsad","sadsadsa"} 
	}, all(pres);
	all["dsad"] = "dasda";
	all["dfdfd"] = "dasda";

	kv.SetValues(all);
	auto readPres = kv.GetMatchPrefix("pre");
	for (auto x : readPres) {
		EXPECT_EQ(pres[x.first], x.second);
	}

}