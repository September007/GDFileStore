#pragma once 
#ifndef CONNECTION_SERVER_HEAD
#define CONNECTION_SERVER_HEAD
#include<httplib.h>
#include<string>
#include<vector>
#include<list>
#include<connection.h>
#include<object.h>
#include<thread>
#include<GDFileStore.h>
#include<config.h>
#include<iostream>
#include<mutex>
#include<map>
#include<shared_mutex>
#include<GDFileStore.h>
using std::string;
using std::vector;
using std::function;
using std::pair;
using std::list;
using namespace httplib;

template<typename T>
class FSCServer {
public:
	virtual ~FSCServer() {}
	FSCServer() {}
	FSCServer(const FSCServer&) = default;
	// host filestore call this with its own pointer
	// in this func,checking fs->fsname and get config about it,then write config info into osd_config waiting for use
	// then create a thread to detecting coming request
	void listen(GDFileStore* fs);
	void stop();
	inline auto is_srv_running() { return bool(is_running); }
protected:
	InfoForOSD osd_config;
	//flag for listen and stop;
	//do not use atomic_bool because somewhere set flag with unkonwn thread commence is dangerous
	bool is_running = false;
	mutex access_to_is_running;
};


template<typename T>
inline void FSCServer<T>::listen(GDFileStore* fs) {
	static_cast<T*>(this)->listen(fs);
}

template<typename T>
inline void FSCServer<T>::stop() {
	static_cast<T*>(this)->stop();
}

/* use http for now
*/
class FSConnnectionServer :public FSCServer<FSConnnectionServer> {
public:
	BOOST_NOINLINE ~FSConnnectionServer()override {}
	FSConnnectionServer() = default;
	FSConnnectionServer(const FSConnnectionServer&) = default;

	void listen(GDFileStore* fs);
	void stop();
	void SrvThreadMain(GDFileStore* fs, httplib::Server* p_srv);
protected:
	//set response for server,return true if success
	static bool ResponseConfig(FSConnnectionServer* fscs, GDFileStore* fs, httplib::Server* p_srv);
	httplib::Server srv;
};

class AsynServer {
	//server info
	InfoForNetNode info;
	shared_mutex access_to_info;

	//srv part
	httplib::Server srv;
	mutex access_to_srv;
	thread srv_t;
	std::atomic_bool srv_shut = false;
	void srvMain();

	//worker
	GD::ThreadPool thread_pool;
	using callback = std::function<void()>;
	//just record them into thread_pool
	////there should be one kind of job
	//list<pair<WOPE, callback>> wopes;
	////there should be one king of callback, when finish rope,then send msg to it's applicant
	//list<pair<ROPE, callback>> ropes;
	//mutex access_to_opes;

	//filestore
	GDFileStore fs;

	//1.client to primary
	//2.primary to replica
	void handleReqWrite(InfoForNetNode from, vector<InfoForNetNode> tos, reqType rt, opeIdType opeId, WOPE wope);
	//node to node
	void handleReqRead(InfoForNetNode from, vector<InfoForNetNode> tos, reqType rt, opeIdType opeId, ROPE rope);
	//1.replica to primary
	//2.other one priemary to replica is on asynClient
	void handleRepWrite(InfoForNetNode from, repType rt, opeIdType opeId);
	//node to node
	void handleRepRead(InfoForNetNode from, repType rt, ROPE_Result result);
	//set up post func
	bool setup_asyn_server_srv(httplib::Server* srv);

public:

	AsynServer(const InfoForNetNode& info,const string& StorePath, const string& fsName = "default_fs")
		:info(info), access_to_info(), access_to_srv(),
		srv_shut(false), srv_t(&AsynServer::srvMain, this),
		thread_pool(std::thread::hardware_concurrency()),
		fs(StorePath,fsName) {
		if (!fs.Mount())
			Error_Exit();
	}
	~AsynServer() {
		shutdown();
		thread_pool.shutdown();
	}
	//shutsown server ,thread
	void shutdown() {
		srv_shut = true;
		{
			std::unique_lock lg(access_to_srv);
			srv.stop();
		}
		if (srv_t.joinable())
			srv_t.join();
	}
};
#endif //CONNECTION_SERVER_HEAD