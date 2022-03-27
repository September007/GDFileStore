#pragma once
#ifndef CONNECTION_HEAD
#define CONNECTION_HEAD
#include<string>
#include<assistant_utility.h>
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
	string connection_str;
	InfoForOSD(const string& name, const string& conn_str) :name(name), connection_str(conn_str) {}
	static bool Read(buffer& buf,const InfoForOSD* inf) {
		::Read(buf, &inf->name);
		::Read(buf, &inf->connection_str);
		return true;
	}
	static void Write(buffer& buf, const  InfoForOSD* inf) {
		::Write(buf, &inf->name);
		::Write(buf, &inf->connection_str);
	}
};

#endif 