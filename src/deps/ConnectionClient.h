#pragma once 
#ifndef CONNECTION_CLIENT_HEAD
#define CONNECTION_CLIENT_HEAD
#include<connection.h>
#include<httplib.h>
#include<object.h>
#include<iostream>
#include<mutex>
#include<map>
using std::string;
using std::vector;
using namespace httplib;
// Curiously recuring template pattern
template<typename T>
class FSCCient {
public:
	virtual ~FSCCient() {
		int i;
		i = 0;
	}
	ConnectionReturnType WriteAndWaitReturn(const GHObject_t& ghobj, Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection = false);
	ConnectionReturnType Read(const GHObject_t& ghobj, Operation r, const InfoForOSD& primary_osd, bool reloadConnection);
	template<typename ...args>
	//send request balah balah
	auto Get(args... ags) {
		if constexpr (!is_same_v<decltype(static_cast<T*>(this)->Get(forward<args>(ags)...)), void>)
			return static_cast<T*>(this)->Get(forward<args>(ags)...);
		else static_cast<T*>(this)->Get(forward<args>(ags)...);
	}
};

template<typename T>
inline ConnectionReturnType FSCCient<T>::WriteAndWaitReturn(const GHObject_t& ghobj, Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection) {
	return  static_cast<T*>(this)->WriteAndWaitReturn(ghobj, ope, osds, reloadConnection);
}

template<typename T>
inline ConnectionReturnType FSCCient<T>::Read(const GHObject_t& ghobj, Operation r, const InfoForOSD& primary_osd, bool reloadConnection) {
	return static_cast<T*>(this)->Read(ghobj, r, primary_osd, reloadConnection);
}

/* use http for now
*/
class FSConnnectionClient:public FSCCient<FSConnnectionClient> {
public:
	//seems like need noinline,or will force skip the func 
	BOOST_NOINLINE ~FSConnnectionClient() override {

	}
	FSConnnectionClient() = default;
	FSConnnectionClient(const FSConnnectionClient&) = default;
	//@Contract_1 first osd mean your leader osd
	//return true if success
	ConnectionReturnType WriteAndWaitReturn(const GHObject_t& ghobj,Operation, const vector<InfoForOSD>& osds, bool reloadConnection = false);
	template<typename ...argsType>
	auto Get(InfoForOSD osd,argsType...args) {
		httplib::Client cl(osd.GetConnectionstr());
		return cl.Get(forward<argsType>(args)...);
	}
	ConnectionReturnType Read(const GHObject_t& ghobj, Operation r, const InfoForOSD& primary_osd, bool reloadConnection);
};

// async client
class AsynClient {
	InfoForNetNode info;
	shared_mutex access_to_info;

	//@dataflow opestate store
	//record the state of ope,trace by opeid which return by writeReq
	std::map<opeIdType, pair<std::condition_variable, WOpeState>> ope_states;
	mutex access_to_opestates;

	//srv part
	httplib::Server srv;
	mutex access_to_srv;
	thread srv_t;
	std::atomic_bool srv_shut = false;
	void srvMain() {
		{
			std::unique_lock lg(access_to_srv);
			auto ret = setup_asyn_client_srv(&srv);
			LOG_EXPECT_TRUE("asynclient", ret);
		}
		while (!srv_shut) {
			InfoForNetNode info;
			{
				std::shared_lock lg(access_to_info);
				info = this->info;
			}
			srv.listen(info.host.c_str(), info.port);
		}
	}
	//server part
	GD::ThreadPool thread_pool;
	//handle the response from primary
	void handleRepWrite(InfoForNetNode from, repType rt, opeIdType opeId);
	//set up post func
	bool setup_asyn_client_srv(httplib::Server* srv);

public:
	AsynClient(const InfoForNetNode& info) :info(info), access_to_info(), access_to_srv(),
		srv_shut(false), srv_t(&AsynClient::srvMain, this),
		thread_pool(std::thread::hardware_concurrency()) {
	}
	~AsynClient() {
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
	//return the opeid for the result lookup
	opeIdType asynWrite(vector<InfoForNetNode> tos, const WOPE& wope);

	opeIdType asynRead(InfoForNetNode to, ROPE rope);
	//wope result lookup
	//no block,just get state
	WOpeState getWopeState(opeIdType opeId);
	//the follow three will block,not suppose use waitWopeBe because it may block forever;

	//this will return if opestate==(failed|be_what)
	WOpeState waitWopeBe(opeIdType opeId, WOpeState be_what);
	WOpeState waitWopeBe_for(opeIdType opeId, WOpeState be_what, chrono::system_clock::duration timeout);
	WOpeState waitWopeBe_until(opeIdType opeId, chrono::system_clock::time_point deadline);

	
};

#endif //CONNECTION_CLIENT_HEAD