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
/* use http for now
*/
class FSConnnectionServer {
public:
	FSConnnectionServer() {}
	// host filestore call this with its own pointer
	void listen(GDFileStore* fs);
	void stop();
	httplib::Server srv;
	static void SrvThreadMain(GDFileStore* fs, httplib::Server* p_srv);
};

#endif //CONNECTION_SERVER_HEAD