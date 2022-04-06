#include<connection.h>
#include<httplib.h>
#include<assistant_utility.h>
using namespace fmt;
bool http_send(InfoForNetNode from, InfoForNetNode to, buffer data) {
	try {
		httplib::Client cli(to.GetConnectionstr());
		buffer package;
		MultiWrite(package, from, to, data);
		auto str = package.universal_str();
		auto ret = cli.Post("/http_send", str.data(), str.length(), "text/plain");
		return ret.error() == httplib::Error::Success;
	}
	catch (std::exception& e) {
		LOG_ERROR("connection", fmt::format("{} got error [{}]", __func__, e.what()));
	}
	return false;
}

bool WriteReq(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, reqType rt) {
	buffer p;
	MultiWrite(p, from, tos, data, rt);
	return http_send(from, tos[0], p);
}

bool WriteRep(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, repType rt) {
	buffer p;
	MultiWrite(p, from, tos, data, rt);
	return http_send(from, tos[0], p);
}
