#include<connection.h>

//@follow http_param pack 3.in http_send
bool http_send(InfoForNetNode from, InfoForNetNode to, buffer data, const string& postname) {
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