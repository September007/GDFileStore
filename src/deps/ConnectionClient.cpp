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

////@follow http_param pack 2.from, tos,rt, oid in Write_Req
//bool WriteReq(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, reqType rt, opeIdType oid) {
//	buffer p;
//	MultiWrite(p,from, tos, data, rt, oid);
//	return http_send(from, tos[0], p, "/writeReq");
//}
//
////@follow http_param pack 3.in Write_Rep
//bool WriteRep(InfoForNetNode from, vector<InfoForNetNode> tos, buffer data, repType rt, opeIdType oid) {
//	buffer p; 
//	MultiWrite(p,tos, data, rt, oid);
//	return http_send(from, tos[0], p, "/writeRep");
//}

//@follow reqWrite pack 1.in AsynClient::asynWrite from,tos,wope,rt,opeid  
opeIdType AsynClient::asynWrite(vector<InfoForNetNode> tos, const WOPE& wope) {
	auto opeid = GetOpeId(wope);
	InfoForNetNode from;
	{
		std::shared_lock lg(access_to_info);
		from = this->info;
	}
	buffer buf;
	auto rt=reqType::Write;
	MultiWrite(buf, from, tos, wope, rt, opeid);
	auto send_result=http_send(from, tos[0], buf, "/reqWrite");
	return send_result ? opeid : "";
}

//@follow http param pack 1.in AsynClient::asynRead from,to,wope,rt,opeid  
opeIdType AsynClient::asynRead(vector<InfoForNetNode> tos, ROPE rope) {
	auto opeId = GetOpeId(rope);
	InfoForNetNode from;
	{
		std::shared_lock lg(access_to_info);
		from = this->info;
	}
	buffer buf;
	auto rt = reqType::Read;
	MultiWrite(buf, from, tos, rope, rt, opeId);
	auto send_result = http_send(from, tos[0], buf, "/asynRead");
	return send_result ? opeId : "";
}

WOpeState AsynClient::getWopeState(opeIdType opeId) {
	WOpeState s;
	{
		unique_lock lg(access_to_wopestates);
		auto& psc = this->wope_states[opeId];
		s = psc.second;
	}
	return s;
}

WOpeState AsynClient::waitWopeBe(opeIdType opeId, WOpeState be_what) {
	unique_lock lg(access_to_wopestates);
	auto& psc = this->wope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait(lg, [&] {return psc.second == be_what; });
	return psc.second;
}

WOpeState AsynClient::waitWopeBe_for(opeIdType opeId, WOpeState be_what,chrono::system_clock::duration timeout) {
	unique_lock lg(access_to_wopestates);
	auto& psc = this->wope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait_for(lg, timeout, [be_what, &psc] {return psc.second == be_what; });
	return psc.second;
}

WOpeState AsynClient::waitWopeBe_until(opeIdType opeId, chrono::system_clock::time_point deadline) {
	unique_lock lg(access_to_wopestates);
	auto& psc = this->wope_states[opeId];
	//@dataflow ope_cv wait
	psc.first.wait_until(lg, deadline);
	return psc.second;
}

pair<ROpeState, ROPE_Result> AsynClient::getRopeState(opeIdType opeId) {
	unique_lock lg(access_to_ropestates);
	auto r = rope_states[opeId].second;
	return r;
}

pair<ROpeState, ROPE_Result> AsynClient::waitRope(opeIdType opeId) {
	unique_lock lg(access_to_ropestates);
	auto& pcr = rope_states[opeId];
	pcr.first.wait(lg);
	return pcr.second;
}

pair<ROpeState, ROPE_Result> AsynClient::waitRope_for(opeIdType opeId, chrono::system_clock::duration timeout) {
	unique_lock lg(access_to_ropestates);
	auto& pcr = rope_states[opeId];
	pcr.first.wait_for(lg,timeout);
	return pcr.second;
}

pair<ROpeState, ROPE_Result> AsynClient::waitRope_until(opeIdType opeId, chrono::system_clock::time_point deadline) {
	unique_lock lg(access_to_ropestates);
	auto& pcr = rope_states[opeId];
	pcr.first.wait_until(lg, deadline);
	return pcr.second;
}

void AsynClient::handleRepWrite(InfoForNetNode from, repType rt, opeIdType opeId) {
	DebugArea(LOG_INFO("asynclient",
		fmt::format("from[{}], reqType: [{}] , opeId : [{}] ", from.GetConnectionstr(), int(rt), opeId)));
	switch (rt) {
		//@dataflow ope_cv set and notify
		case repType::primaryJournalWrite:
		{
			unique_lock lg(access_to_wopestates);
			auto& state = wope_states[opeId];
			state.second = WOpeState::onJournal;
			state.first.notify_all();
			break;
		}
		case repType::primaryDiskWrite:
		{
			unique_lock lg(access_to_wopestates);
			auto& state = wope_states[opeId];
			state.second = WOpeState::onDisk;
			state.first.notify_all();
			break;
		}
		default:
			LOG_ERROR("asynclient", fmt::format("this doesn't should recieve reqType[{}] ", int(rt)));
			break;
	}
}

void AsynClient::handleRepRead(InfoForNetNode from, repType rt, opeIdType opeId,ROPE_Result result) {
	switch (rt) {
		case repType::primaryRead:
		{
			unique_lock lg(access_to_ropestates);
			auto& pcp = rope_states[opeId];
			//@emergency unpack
			pcp.second = make_pair(ROpeState::success, result);
		}
			break;
		default:
			break;
	}
}

bool AsynClient::setup_asyn_client_srv(httplib::Server* srv) {
	if (!srv)return false;
	try {
		auto pthis = this;
		//@dataflow request repWrite
		srv->Post("/repWrite", [pthis](const httplib::Request& req, httplib::Response& rep) {
			buffer buf(req.body);
			//@dataflow RepWrite , unpack param as ReqWrite did
			auto _from = Read<InfoForNetNode>(buf);
			auto _to = Read<InfoForNetNode>(buf);
			auto http_data = Read<buffer>(buf);
			auto from = Read<InfoForNetNode>(http_data);
			auto rt = Read<repType>(http_data);
			auto opeId = Read<opeIdType>(http_data);
			pthis->thread_pool.enqueue([pthis=pthis,from=move(from),rt=rt,opeId=move(opeId)] {
				pthis->handleRepWrite(from, rt, opeId);
				});
			});
		return true;
	}
	catch (std::exception& e) {
		LOG_ERROR("asynclient", fmt::format("got error[{}]", e.what()));
	}
	return false;
}
