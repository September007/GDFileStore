#pragma once
#ifndef CONNECTION_HEAD
#define CONNECTION_HEAD
#include<string>
//#include<object.h>
#include<assistant_utility.h>
#include<config.h>
#include<httplib.h>
#include<mutex>
#include<shared_mutex>
#include<thread_pool.h>
using std::string;
using std::mutex;
using std::shared_mutex;
using std::map;
using std::shared_lock;
using std::unique_lock;
enum class ConnectionReturnType {
	success = 0,
	fail_of_primary,
	fail_of_replica,
	fail_anyway
};
class InfoForOSD {
public:
	string name;
	string host;
	int port;
	InfoForOSD(const string& name="", const string& host="",int port=0) :name(name), host(host), port(port) {}
	bool operator==(const InfoForOSD& ifs)const { return name == ifs.name && host == ifs.host && port == ifs.port; }
	auto GetES()const  { return make_tuple(&name, &host, &port); }
	auto GetConnectionstr()const { return fmt::format("{}:{}", host, port); }
};
//network connection node info
using InfoForNetNode = InfoForOSD;
inline auto GetOSDConfig(const string& osd_name) {
	auto http_config = GetConfig(osd_name, "http_server");
	{
		if (http_config.empty()||http_config["addr"].empty()||http_config["port"].empty()) {
			LOG_ERROR("config", fmt::format("config {}:{} get null", osd_name, "http_server"));
			Error_Exit();
		}
	}
	auto host = http_config["addr"].get<string>();
	auto port = http_config["port"].get<int>();
	if (host == "" || port == int{}) {
		LOG_ERROR("server", fmt::format("read {}::server config failed", osd_name));
		return make_pair<bool, InfoForOSD>(false, InfoForOSD());
	}
	else
		return make_pair<bool, InfoForOSD>(true, InfoForOSD(osd_name, host, port));
}
//@emergency add drawback
enum class reqType {
	Write,  // osd distinguish either it's as primary osd or not by mathcing it's info in incoming  param tos,if it's on first,then it's primary
	Read,	
	WriteDrawBack,//drawback work as Write
	primaryRecover
};
enum class repType {
	replicaJounralWrite,
	replicaDiskWrite,
	//primary's msg should wait until replica's finish
	primaryJournalWrite,
	primaryDiskWrite, 

	replicaJournalRecover,
	replicaDiskRecover,
	primaryJournalRecover,
	primaryDiskRecover,

	primaryRead
};
// indicate the process of ope
enum class WOpeState {
	other,
	failed,
	onJournal,
	onDisk
};
enum class ROpeState {
	other,
	failed,
	success
};
union OpeState {
	WOpeState wope;
	ROpeState rope;
};
bool http_send(InfoForNetNode from, InfoForNetNode to, buffer data, const string& postname="/http_send");
//bool WriteReq(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, reqType rt,opeIdType oid);
//bool WriteRep(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, repType rt,opeIdType oid);

#endif 