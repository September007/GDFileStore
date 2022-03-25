#pragma once 
#ifndef CONNECTION_CLIENT_HEAD
#define CONNECTION_CLIENT_HEAD
#include<httplib.h>
#include<string>
#include<object.h>
#include<vector>
#include<assistant_utility.h>
using std::string;
using std::vector;
using namespace httplib;
enum class ConnectionReturnType {
	success=0,
	fail_of_primary,
	fail_of_replica,
};
class InfoForOSD {
public:
	string name;
	string connection_str;
	InfoForOSD(const string& name, const string& conn_str) :name(name), connection_str(conn_str) {}
	static bool Read(buffer &buf,InfoForOSD*info){
		::Read(buf, &info->name);
		::Read(buf, &info->connection_str);
	}
	static void Write(buffer& buf, const InfoForOSD*info) {
		::Write(buf, &info->name);
		::Write(buf, &info->connection_str);
	}
};
/* use http for now
*/
class FSConnnectionClient {
public:
	//@Contract_1 first osd mean your leader osd
	//return true if success
	ConnectionReturnType WriteAndWaitReturn(const GHObject_t& ghobj, const  Operation&, const vector<InfoForOSD>& osds, bool reloadConnection = false);
};

#endif //CONNECTION_CLIENT_HEAD