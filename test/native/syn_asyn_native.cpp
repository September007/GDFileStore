
#include<ConnectionClient.h>
#include<ConnectionServer.h>
//#include<benchmark\benchmark.h>
namespace benchmark {
	class State {
	public:
		vector<int> its;
		vector<int> parms;
		int range(int i) { return parms[i]; }
		auto begin() {
			return its.begin();
		}
		auto end() {
			return its.end();
		}
	};
}

//svr start before client
mutex commence;
condition_variable svr_started;

mutex finish;
condition_variable asyn_client_finished;
auto srv_info = InfoForNetNode("test_server", "127.0.0.1", 8080);
auto cli_info = InfoForNetNode("test_client", "127.0.0.1", 8081);
struct test_data {
	vector<vector<string>> dic_ope_block;
	static test_data Get(int ope_cnt, int block_cnt, int block_sz) {
		test_data ret{ vector<vector<string>>(ope_cnt,vector<string>(block_cnt)) };
		// simplify data
		for (auto& vs : ret.dic_ope_block)
			for (int i = 0; i < vs.size(); ++i)
				vs[i] = string(block_sz, '0' + (i % 10));
		return ret;
	}
	static auto& cacheInstance(int o, int b, int s) {
		using key = pair<int, pair<int, int>>;
		static map<key, test_data>cache;
		auto k = key{ o,{b,s } };
		auto p = cache.find(k);
		if (p != cache.end())
			return p->second;
		else
			return cache[k] = Get(o, b, s);
	}
};
void client_main(benchmark::State& state) {

	//wait for server start
	this_thread::sleep_for(chrono::milliseconds(100));
	shared_ptr<FSConnnectionClient> cli(new FSConnnectionClient);
	GHObject_t ghobj(HObject_t(Object_t("test_obj"), Snapid_t()), 0, shard_id_t::NO_SHARD());
	auto x = OperationWrapper::WriteWrapper(ghobj, "tring writing something\n");
	vector<InfoForOSD> osds = {
		InfoForOSD("target_primary_osd","127.0.0.1",8080)
	};
	auto r = cli->WriteAndWaitReturn(ghobj, x, osds);
	//shut down server
	cli->Get(osds[0], "/stop");
}
void syn_server_main() {
	try {
		{
			lock_guard lg(commence);
			svr_started.notify_all();
		}
		shared_ptr<FSConnnectionServer> srv(new FSConnnectionServer);
		GDFileStore fs("./tmp/http_connection_/fs", "target_primary_osd");
		srv->listen(&fs);
		//when srv.listen quit, srv already stop
	}
	catch (std::exception& e) {
		LOG_ERROR("test", fmt::format("catch error [{}] ", e.what()));
	}
	int i = 0;
	i++;
}
static void Syn_Write_Test(benchmark::State& state) {
	//unpack parms
	auto ope_cnt = state.range(0);
	auto block_cnt = state.range(1);
	auto block_size = state.range(2);

	auto& cache_data = test_data::cacheInstance(ope_cnt, block_cnt, block_size);
	vector<string> opes_datas(ope_cnt);
	for (int i = 0; i < ope_cnt; ++i)
		for (auto& block_data : cache_data.dic_ope_block[i])
			opes_datas[i].append(block_data);
	//set up server
	thread srv_t(syn_server_main);
	//wait server start;
	{
		unique_lock lg(commence);
		svr_started.wait(lg);
	}
	FSConnnectionClient  cli;
	vector<GHObject_t> ghobjs(ope_cnt);
	vector<Operation> wopes(ope_cnt);
	for (int i = 0; i < ope_cnt; ++i) {
		ghobjs[i].hobj.oid.name = fmt::format("test_obj::{}", i);
		wopes[i] = OperationWrapper::WriteWrapper(ghobjs[i], opes_datas[i]);
	}
	vector<InfoForOSD> osds = { srv_info };
	//run tests
	for (auto _ : state) {
		for (int i = 0; i < ope_cnt; ++i)
			cli.WriteAndWaitReturn(ghobjs[i], wopes[i], osds);
	}
	//shut down server
	cli.Get(osds[0], "/stop");
	srv_t.join();
}


static void Asyn_Write_Test(benchmark::State& state) {
	//unpack parms
	auto ope_cnt = state.range(0);
	auto block_cnt = state.range(1);
	auto block_size = state.range(2);

	auto& cache_data = test_data::cacheInstance(ope_cnt, block_cnt, block_size);

	thread svr_t([&] {
		AsynServer svr(srv_info, "./tmp/fs", "target_primary_osd");
		{
			unique_lock lg(commence);
			//awaken client
			svr_started.notify_all();
		}
		{
			unique_lock lg(finish);
			//wait client quit
			asyn_client_finished.wait(lg);
		}
		});
	{
		unique_lock lg(commence);
		svr_started.wait(lg);
	}
	vector<WOPE> opes;// (ope_cnt);
	vector<opeIdType> opeIds(ope_cnt);
	vector<InfoForNetNode> osds{ srv_info };
	vector<int> block_nums;
	for (int i = 0; i < block_cnt; ++i)
		block_nums.push_back(i);

	AsynClient cli(cli_info);
	//init data
	for (int i = 0; i < ope_cnt; ++i) {
		auto gh = GHObject_t(HObject_t(Object_t(fmt::format("test_obj::{}", i)))), new_gh = gh;
		new_gh.generation++;
		opes.push_back(WOPE(gh, new_gh, vector<WOPE::opetype>(block_cnt, WOPE::opetype::Insert),
			block_nums, cache_data.dic_ope_block[i]));
	}
	for (auto _ : state) {
		for (int i = 0; i < ope_cnt; ++i) {
			opeIds[i] = cli.asynWrite(osds, opes[i]);
		}
		for (int i = ope_cnt - 1; i >= 0; --i)
			cli.waitWopeBe(opeIds[i], WOpeState::onDisk);
	}
	// notify server to quit
	{
		unique_lock lg(finish);
		asyn_client_finished.notify_all();
	}
	if (svr_t.joinable())
		svr_t.join();
}

#define min_ope_cnt 10
#define max_ope_cnt 100
#define min_block_cnt 1
#define max_block_cnt 1
#define min_block_size 100
#define max_block_size 100

	benchmark::State state;
int main(int argc, char** argv) {
	//init data
	for (int i = min_ope_cnt; i <= max_ope_cnt; i *= 10)
		for (int j = min_block_cnt; j <= max_block_cnt; j *= 10)
			for (int k = min_block_size; k <= max_block_size; k *= 10)
				test_data::cacheInstance(i, j, k);
	state.its.resize(5);
	state.parms = { 100,1,100 };
	Syn_Write_Test(state);
	Asyn_Write_Test(state);

	return 0;
}

