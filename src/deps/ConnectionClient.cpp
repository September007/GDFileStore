#include<connection.h>
#include<ConnectionClient.h>
// create link, push param, wait return
ConnectionReturnType FSConnnectionClient::WriteAndWaitReturn(
	const GHObject_t& ghobj,  Operation ope, const vector<InfoForOSD>& osds, bool reloadConnection) {
	if (osds.size() == 0)
		return ConnectionReturnType::fail_of_primary;
	httplib::Client cl(osds[0].GetConnectionstr());
	if(! cl.is_valid())
		return ConnectionReturnType::fail_anyway;
	buffer buf;
	int osd_cnt = osds.size();
	// code for params
	Write<Operation>(buf, &ope);
	Write(buf, &osd_cnt);
	WriteArray(buf, const_cast<InfoForOSD*>(& osds[0]), osds.size());
	Write(buf, &reloadConnection);
	auto str = buf.universal_str();
	auto str_len = str.size();
	////auto res = cl.Get("/hello");
	//{
	//	auto b = buf.universal_str();
	//	auto buf = make_shared<buffer>(b);
	//	auto ope = Read<Operation>(*(buf.get()));
	//	auto osd_len = Read<int>(*(buf.get()));
	//	auto osds = ReadArray<InfoForOSD>(*(buf.get()), osd_len);
	//	auto reload_connection = Read<bool>(*(buf.get()));
	//}
	httplib::Result res = cl.Post("/write",str_len, [&](size_t offset, size_t length, DataSink& sink) {
		sink.write(str.data() + offset, length);
		return true; // return 'false' if you want to cancel the request.
		},
		"text/plain");
	if (res.error() != Error::Success)
		return ConnectionReturnType::fail_anyway;
	httplib::Response r=res.value();
	auto resultBody = r.body;
	if (resultBody == "success")
		return ConnectionReturnType::success;
	else
		return ConnectionReturnType::fail_anyway;
}

ConnectionReturnType FSConnnectionClient::Read
(const GHObject_t& ghobj, Operation r, const InfoForOSD& primary_osd, bool reloadConnection) {

	return ConnectionReturnType();
}
#include<httplib.h>
#include<assistant_utility.h>
using namespace fmt;
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

//@follow http_param pack 2.in Write_Req
bool WriteReq(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, reqType rt, opeIdType oid) {
	buffer p;
	MultiWrite(p, tos, data, rt, oid);
	return http_send(from, tos[0], p, "/writeReq");
}

//@follow http_param pack 3.in Write_Rep
bool WriteRep(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, repType rt, opeIdType oid) {
	buffer p; 
	MultiWrite(p,tos, data, rt, oid);
	return http_send(from, tos[0], p, "/writeRep");
}

opeIdType AsynClient::asynWrite(vector<InfoForNetNode> tos, const WOPE& wope) {
	auto opeid = GetOpeId(wope);
	InfoForNetNode from;
	{
		std::shared_lock lg(access_to_info);
		from = this->info;
	}
	buffer buf;
	MultiWrite(buf, tos, wope);
	WriteReq(from, tos, buf, reqType::clientWrite, opeid);
	return opeid;
}

OpeState AsynClient::getWopeState(opeIdType opeId) {
	OpeState s;
	{
		shared_lock lg(access_to_opestates);
		auto& psc = this->ope_states[opeId];
		s = psc.second;
	}
	return s;
}

OpeState AsynClient::waitWopeBe(opeIdType opeId, OpeState be_what) {
	unique_lock lg(access_to_opestates);
	auto& psc = this->ope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait(lg, [&] {return psc.second == be_what; });
	return psc.second;
}

OpeState AsynClient::waitWopeBe_for(opeIdType opeId, OpeState be_what,
	chrono::system_clock::duration timeout) {
	unique_lock lg(access_to_opestates);
	auto& psc = this->ope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait_for(lg, timeout);
	return psc.second;
}

OpeState AsynClient::waitWopeBe_until(opeIdType opeId, OpeState be_what
	, chrono::system_clock::time_point deadline) {
	unique_lock lg(access_to_opestates);
	auto& psc = this->ope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait_until(lg, deadline);
	return psc.second;
}

void AsynClient::handleReqWrite(InfoForNetNode from, repType rt, opeIdType opeId) {
	DebugArea(LOG_INFO("asynclient",
		fmt::format("from[{}], reqType: [{}] , opeId : [{}] ", from.GetConnectionstr(), int(rt), opeId)));
	switch (rt) {
		//@dataflow ope_cv set and notify
		case primaryJournalWrite:
		{
			unique_lock lg(access_to_opestates);
			auto& state = ope_states[opeId];
			state.second = OpeState::onJournal;
			state.first.notify_all();
			break;
		}
		case primaryDiskWrite:
		{
			unique_lock lg(access_to_opestates);
			auto& state = ope_states[opeId];
			state.second = OpeState::onDisk;
			state.first.notify_all();
			break;
		}
		default:
			LOG_ERROR("asynclient", fmt::format("this doesn't should recieve reqType[{}] ", rt));
			break;
	}
}

bool AsynClient::setup_asyn_client_srv(httplib::Server* srv) {
	if (!srv)return false;
	try {
		auto pthis = this;
		srv->Post("/RepWrite", [pthis](const httplib::Request& req, httplib::Response& rep) {
			buffer buf(req.body);
			//@follow RepWrite , unpack param as ReqWrite did

			pthis->thread_pool.enqueue([pthis]() {pthis->handleReqWrite()})
			})
	}
	catch (std::exception& e) {
		LOG_ERROR("asynclient", fmt::format("got error[{}]", e.what()));
	}
	return false;
}
