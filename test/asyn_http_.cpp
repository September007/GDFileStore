#include<test_head.h>
#include<ConnectionClient.h>
#include<ConnectionServer.h>

using namespace std;
auto srv_info = InfoForNetNode("test_server", "127.0.0.1", 8080);
auto cli_info = InfoForNetNode("test_client", "127.0.0.1", 8081);
mutex m_;
condition_variable cv;
void cliMain() {
	this_thread::sleep_for(chrono::milliseconds(100));
	auto block_len = GetconfigOverWrite(4096, "FileStore", "", "block_size");
	AsynClient cli(cli_info);
	auto gh = GHObject_t(HObject_t(Object_t("ghobj")), 1);
	auto new_gh = gh; new_gh.generation++;
	auto types = vector<WOPE::opetype>{ WOPE::opetype::Insert,WOPE::opetype::Insert ,WOPE::opetype::Insert };
	auto block_nums = vector<int>{0,1,2};
	auto block_datas = vector<string>{
		string(block_len,'a'),
		string(block_len,'b'),
		string(block_len,'c')
	};
	WOPE wope = WOPE(gh, new_gh, types, block_nums, block_datas);
	//asyn write,return key for result looking
	auto opeId = cli.asynWrite({ srv_info }, wope);

	auto timeout = chrono::milliseconds(200000);
	//result looking up
	auto onjournal = cli.waitWopeBe_for(opeId, WOpeState::onJournal, timeout);
	auto ondisk = cli.waitWopeBe_for(opeId, WOpeState::onDisk, timeout);
	
	//check results
	EXPECT_EQ(onjournal, WOpeState::onJournal);
	EXPECT_EQ(ondisk, WOpeState::onDisk);

	auto ropeId = cli.asynRead({ srv_info }, ROPE(new_gh, { 0,1,2 }));
	auto rresult = cli.waitRope_for(ropeId, timeout);
	EXPECT_EQ(rresult.first, ROpeState::success);

	{
		unique_lock lg(m_);
		cv.notify_all();
	}
}
void svrMain() {
	unique_lock lg(m_);
	AsynServer svr(srv_info, "./tmp/fs", "target_primary_osd");
	cv.wait(lg);
}
TEST(asyn_http, basic) {
	thread  svr(svrMain);
	httplib::Client c(srv_info.GetConnectionstr());
	string s = "Dadsa";
	//auto r = c.Post("/reqWrite", s.c_str(), s.length(), "text/plain");

	thread cli(cliMain);
	cli.join();
	svr.join();


}