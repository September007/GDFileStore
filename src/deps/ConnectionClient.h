#pragma once 
#ifndef CONNECTION_CLIENT_HEAD
#define CONNECTION_CLIENT_HEAD
#include<connection.h>
#include<httplib.h>
#include<object.h>
#include<iostream>
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

#endif //CONNECTION_CLIENT_HEAD