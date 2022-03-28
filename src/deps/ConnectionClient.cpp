#include<connection.h>
#include<ConnectionClient.h>

ConnectionReturnType FSConnnectionClient::WriteAndWaitReturn(
	const GHObject_t& ghobj,  Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection) {
	if (osds.size() == 0)
		return ConnectionReturnType::fail_of_primary;
	httplib::Client cl("127.0.0.1",8080);
	if(! cl.is_valid())
		return ConnectionReturnType::fail_anyway;
	buffer buf;
	int osd_cnt = osds.size();
	// code for params
	Write<Operation>(buf, &ope);
	Write(buf, &osd_cnt);
	WriteArray(buf, &osds[0], osds.size());
	//for (int i = 0; i < osd_cnt; ++i)
	//	Write(buf, &osds[i]);
	Write(buf, &reloadConnection);
	auto str = buf.universal_str();
	auto str_len = str.size();
	//auto res = cl.Get("/hello");
	httplib::Result res = cl.Post("/write",str_len, [str=move(str)](size_t offset, size_t length, DataSink& sink) {
		sink.write(str.data() + offset, length);
		return true; // return 'false' if you want to cancel the request.
		},
		"text/plain");
	httplib::Response r=res.value();
	auto resultBody = r.body;
	if (resultBody == "success")
		return ConnectionReturnType::success;
	else
		return ConnectionReturnType::fail_anyway;
}
