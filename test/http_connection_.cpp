#include<test_head.h>
#include<ConnectionClient.h>
#include<ConnectionServer.h>
#include<object.h>

void client_main() {
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
	cout << fmt::format("client recieve {},\n({} mean success)\n", int(r), int(ConnectionReturnType::success));
}

void server_main() {
	try {

		shared_ptr<FSConnnectionServer> srv(new FSConnnectionServer);
		GDFileStore fs("./tmp/http_connection_/fs", "target_primary_osd");
		EXPECT_TRUE(fs.Mount());
		srv->listen(&fs);
		//when srv.listen quit, srv already stop
	}
	catch (std::exception& e) {
		LOG_ERROR("test", fmt::format("catch error [{}] ", e.what()));
	}
	int i = 0; 
	i++;

}
TEST(http_connection, main) {
	try {
		thread server_t(server_main);
		thread client_t(client_main);
		client_t.join();
		server_t.join();
	}
	catch (std::exception& e) {
		LOG_ERROR("test", fmt::format("catch error [{}] ", e.what()));
	}
}