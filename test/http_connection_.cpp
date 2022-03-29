#include<test_head.h>
#include<ConnectionClient.h>
#include<ConnectionServer.h>
#include<object.h>

void client_main() {
	//wait for server start
	this_thread::sleep_for(chrono::seconds(1));
	FSConnnectionClient cli;
	GHObject_t ghobj(HObject_t(Object_t("test_obj"),Snapid_t()), 0, shard_id_t::NO_SHARD());
	auto x = OperationWrapper::WriteWrapper(ghobj, "tring writing something");
	vector<InfoForOSD> osds = {
		InfoForOSD("target_primary_osd","127.0.0.1:8080")
	};
	
	auto r=cli.WriteAndWaitReturn(ghobj,x, osds);
	//shut down server
	cli.Get(osds[0],"/stop");
	cout << fmt::format("client recieve {}",int(r));
	cli.Get(osds[0], "/");
}

void server_main() {
	FSConnnectionServer srv;
	GDFileStore fs("./tmp/http_connection_/fs", "target_primary_osd");
	srv.listen(&fs);
	//wait for srv working, because when process slip over there,srv and fs will destruct, child thread would fail
	this_thread::sleep_for(chrono::seconds(100));
}

TEST(http_connection, main) {
	thread server_t(server_main);
	thread client_t(client_main); client_t.join();
	server_t.join();
}