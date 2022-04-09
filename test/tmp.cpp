#include<test_head.h>
#include<filebuffer.h>
#include<assistant_utility.h>
#define head TMP
#include<vector>
#include<connection.h>
#include<GDKeyValue.h>
using namespace std;
TEST(RocksKV, attr_Set) {
	RocksKV kv;
	kv.LoadDB();
	GHObject_t gh(HObject_t(Object_t("name"),1),2,shard_id_t(3));
	GHObject_t::Attr_Type attr;
	attr.serials_list = { 1,2,3 };
	kv.SetAttr(gh, attr);
	auto at = kv.GetAttr(gh).first;
	auto l = at.serials_list;
}

TEST(serialize, WOPE) {

	auto gh = GHObject_t(HObject_t(Object_t("ghobj")), 1);
	auto new_gh = gh; new_gh.generation++;
	auto types = vector<WOPE::opetype>{};
	auto block_nums = vector<int>{};
	auto block_datas = vector<string>{};
	WOPE wope = WOPE(gh, new_gh, types, block_nums, block_datas);
	{
		buffer buf;
		Write(buf, &gh);
		auto r = Read<GHObject_t>(buf);
		r.generation++;
	}
	{
		buffer buf;
		Write(buf, &types);
		auto r = Read<decltype(types)>(buf);
		r.size();
	}
	{
		buffer buf;
		MultiWrite(buf, wope.types, wope.ghobj, wope.new_ghobj, wope.block_nums, wope.block_datas);
		auto tps = Read<decltype(types)>(buf);
		auto g = Read<decltype(gh)>(buf);
		auto ng = Read<decltype(gh)>(buf);
		auto bn = Read<decltype(block_nums)>(buf);
		auto bd = Read<decltype(block_datas)>(buf);

	}
	buffer buf;
	Write(buf, &wope);
	auto r = Read<WOPE>(buf);

	r.block_datas.push_back("");
}
