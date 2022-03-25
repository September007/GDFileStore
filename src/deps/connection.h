#pragma once
#ifndef CONNECTION_HEAD
#define CONNECTION_HEAD
#include<string>
using std::string;
enum class ConnectionReturnType {
	success = 0,
	fail_of_primary,
	fail_of_replica,
};
class InfoForOSD {
public:
	string name;
	string connection_str;
	InfoForOSD(const string& name, const string& conn_str) :name(name), connection_str(conn_str) {}
};

#endif 