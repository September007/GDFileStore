#pragma once 
#ifndef CONNECTION_SERVER_HEAD
#define CONNECTION_SERVER_HEAD
#include<httplib.h>
#include<string>
#include<vector>
#include<connection.h>
#include<object.h>
#include<thread>
#include<GDFileStore.h>
#include<config.h>
using std::vector;
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
class FSConnnectionServer:public FSCServer<FSConnnectionServer> {
public:
	BOOST_NOINLINE ~FSConnnectionServer()override {}
	FSConnnectionServer() = default;
	FSConnnectionServer(const FSConnnectionServer&) = default;

	void listen(GDFileStore* fs);
	void stop();
	void SrvThreadMain(GDFileStore* fs, httplib::Server* p_srv);
protected:
	//set response for server,return true if success
	static bool ResponseConfig(FSConnnectionServer*fscs, GDFileStore* fs, httplib::Server* p_srv);
	httplib::Server srv;
};

#endif //CONNECTION_SERVER_HEAD