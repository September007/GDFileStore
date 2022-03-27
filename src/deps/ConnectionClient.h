#pragma once 
#ifndef CONNECTION_CLIENT_HEAD
#define CONNECTION_CLIENT_HEAD
#include<connection.h>
#include<httplib.h>
#include<object.h>
using std::string;
using std::vector;
using namespace httplib;
/* use http for now
*/
class FSConnnectionClient {
public:
	//@Contract_1 first osd mean your leader osd
	//return true if success
	ConnectionReturnType WriteAndWaitReturn(const GHObject_t& ghobj,Operation, const vector<InfoForOSD>& osds, bool reloadConnection = false);
	template<typename ...argsType>
	auto Get(InfoForOSD osd,argsType...args) {
		httplib::Client cl(osd.connection_str);
		return cl.Get(forward<argsType>(args)...);
	}
};

#endif //CONNECTION_CLIENT_HEAD