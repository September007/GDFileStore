#include<test_head.h>
#include<filebuffer.h>
#include<assistant_utility.h>
#define head TMP
#include<vector>
#include<connection.h>
#include<GDKeyValue.h>
using namespace std;
TEST(filebuffer, all) {
	RocksKV kv;
	kv.LoadDB();
	GHObject_t gh(HObject_t(Object_t("name"),1),2,shard_id_t(3));
	GHObject_t::Attr_Type attr;
	attr.serials_list = { 1,2,3 };
	kv.SetAttr(gh, attr);
	auto at=kv.GetAttr(gh);
	auto l = at.serials_list;
}

