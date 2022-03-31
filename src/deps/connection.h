#pragma once
#ifndef CONNECTION_HEAD
#define CONNECTION_HEAD
#include<string>
#include<object.h>
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
	InfoForOSD(const string& name="", const string& conn_str="") :name(name), connection_str(conn_str) {}
	bool operator==(const InfoForOSD& ifs)const { return name == ifs.name && connection_str == ifs.connection_str; }
	auto GetES() { return make_tuple(&name, &connection_str); }
};
// Curiously recuring template pattern
template<typename T>
class FSCCient {
public:
	ConnectionReturnType WriteAndWaitReturn(const GHObject_t& ghobj, Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection = false);
};

#endif 
template<typename T>
inline ConnectionReturnType FSCCient<T>::WriteAndWaitReturn(const GHObject_t& ghobj, Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection) {
	return  static_cast<T*>(this)->WriteAndWaitReturn(ghobj, ope, osds, reloadConnection);
}

template<typename T>
class FSCServer {

};
