#pragma once
#ifndef CONNECTION_HEAD
#define CONNECTION_HEAD
#include<string>
#include<object.h>
#include<assistant_utility.h>
#include<config.h>
using std::string;
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
	auto GetES() { return make_tuple(&name, &host, &port); }
	auto GetConnectionstr()const { return fmt::format("{}:{}", host, port); }
};

inline auto GetOSDConfig(const string& osd_name) {
	auto http_config = GetConfig(osd_name, "http_server");
	{
		if (http_config.empty()) {
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
#endif 