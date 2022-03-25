#include<ConnectionClient.h>

ConnectionReturnType FSConnnectionClient::WriteAndWaitReturn(
	const GHObject_t& ghobj, const Operation&ope, const vector<InfoForOSD>& osds, bool reloadConnection) {
	httplib::Client cl(osds[0].connection_str);
	buffer buf;
	// code for params
	Write<Operation>(buf, (Operation*) & ope);
	WriteArray(buf, &osds[0],1);
	Write(buf, &reloadConnection);
	auto str = buf.universal_str();
	auto res = cl.Post("/write",str.size(), [str=move(str)](size_t offset, size_t length, DataSink& sink) {
		sink.write(str.data() + offset, length);
		return true; // return 'false' if you want to cancel the request.
		},
		"text/plain");
	return ConnectionReturnType::fail_of_primary;
}
