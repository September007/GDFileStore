#include<ConnectionServer.h>

void FSConnnectionServer::listen(GDFileStore* fs) {
	if (this->is_srv_running())return;
	// no matter what task the server was working on,shut it down
	if (srv.is_running()) {
		LOG_WARN("server", fmt::format("shut working server down"));
		srv.stop();
	}
	if (!this->srv.is_valid()) {
		LOG_ERROR("server", "server is invalid");
	};
	// not use lock_gurad because lifetime or underlying executionof listenThread is half crossing exclusive area
	thread x;
	access_to_is_running.lock();
	try {
		thread listenThread(&FSConnnectionServer::SrvThreadMain, this, fs, &this->srv);
		is_running = true;
		x = move(listenThread);
	}
	catch (std::exception& e) {
		LOG_ERROR("server", fmt::format("server::listen get error[{}]", e.what()));
	}
	access_to_is_running.unlock();
	if(x.joinable())
		x.join();
}

void FSConnnectionServer::stop() {
	//dont konw if this is thread-safe
	srv.stop();
	is_running = false;
}

void FSConnnectionServer::SrvThreadMain(GDFileStore* fs, httplib::Server* p_srv) {
	auto config_result = ResponseConfig(this, fs, p_srv);
	if (!config_result) {
		LOG_ERROR("server", fmt::format("{}::server::{} start failed beacuse of config fialed", fs->fsname, __func__));
		return;
	}
	auto& srv = *p_srv;
	auto osd_info = GetOSDConfig(fs->GetOsdName());
	if (!osd_info.first) {
		LOG_ERROR("server", fmt::format("get {}.server config failed", fs->GetOsdName()));
		return;
	}
	//listen on specified host:port
	srv.listen(osd_info.second.host.c_str(), osd_info.second.port);
}

bool FSConnnectionServer::ResponseConfig(FSConnnectionServer* fscs, GDFileStore* fs, httplib::Server* p_srv) {
	if (!p_srv) {
		LOG_ERROR("sever", fmt::format("get null server"));
		return false;
	}
	// refer
	auto& srv = *p_srv;
	srv.Get("/stop", [&](const Request& req, Response& res0)->bool {srv.stop(); return true; });
	srv.Get("/hello", [](const Request& req, Response& res)->bool {
		res.body = "hello"; return true;
		});
	srv.Post("/write", [fs = fs](const Request& req, Response& res, const ContentReader& content_reader)->bool {
		string content;
		if (req.is_multipart_form_data()) {
			//this is not adopt for now
			// NOTE: `content_reader` is blocking until every form data field is read
			MultipartFormDataItems files;
			content_reader(
				[&](const MultipartFormData& file) {
					content.append(file.content);
					return true;
				},
				[&](const char* data, size_t data_length) {
					content.append(data, data_length);
					return true;
				});
		}
		else {
			content_reader([&](const char* data, size_t data_length) {
				content.append(data, data_length);
				return true;
				});
		}
		// decode refer to code from cli.write
		auto buf = make_shared< buffer>(content);

		auto ope = Read<Operation>(*(buf.get()));
		auto osd_len = Read<int>(*(buf.get()));
		auto osds = ReadArray<InfoForOSD>(*(buf.get()), osd_len);
		auto reload_connection = Read<bool>(*(buf.get()));
		bool result = false;
		try {
			 result = fs->HandleWriteOperation(ope, osds);
		}
		catch (std::exception& e) {
			cout << fmt::format("catch error[{}]", e.what());
		}
		if (result == true) {
			res.body = "success";
		}
		return true;
		});
	srv.Post("/read", [](const Request& req, Response& rep) {

		});
	return true;
}

void AsynServer::srvMain() {
	{
		std::unique_lock lg(access_to_srv);
		auto ret = setup_asyn_server_srv(&srv);
		LOG_EXPECT_TRUE("asynserver", ret);
	}
	while (!srv_shut) {
		InfoForNetNode info;
		{
			std::shared_lock lg(access_to_info);
			info = this->info;
		}
		bool b=srv.listen(info.host.c_str(), info.port);
		LOG_EXPECT_TRUE("asyn", b);
	}
}

//two kind of write request, 
// 1. from client to primary 
// 2. from parimary to replica
void AsynServer::handleReqWrite(InfoForNetNode from, vector<InfoForNetNode> tos, 
	reqType rt, opeIdType opeId, WOPE wope) {
	switch (rt) {
		case reqType::Write:
		{
			{
				unique_lock lg(access_to_info);
				LOG_INFO("asynserver", fmt::format("[svr:[{}]] handle request ClientWrite from [client:[{}]]",
					info.GetConnectionstr(), from.GetConnectionstr()));
			}
			//@emergency to send msg to filestore

		}
			break;
		default:
			LOG_ERROR("asynserver", fmt::format("[svr:[{}]] handle unexpected write request from [cli:[{}]]",
				info.GetConnectionstr(), from.GetConnectionstr()));
			break;
	}
}

//two kind of read request, 
// 1. from client to primary 
// 2. from parimary to replica   this may come from primary's data recover
void AsynServer::handleReqRead(InfoForNetNode from, vector<InfoForNetNode> tos,
	reqType rt, opeIdType opeId, ROPE rope) {
	switch (rt) {
		case reqType::Read:
		{
			{
				LOG_ERROR("asynserver", fmt::format("[svr:[{}]] handle clientRead request from [cli:[{}]]",
					info.GetConnectionstr(), from.GetConnectionstr()));
			}
			//@emergency to send msg to filestore

		}
			break;
		default:
			LOG_ERROR("asynserver", fmt::format("[svr:[{}]] handle unexpected read request from [cli:[{}]]",
				info.GetConnectionstr(), from.GetConnectionstr()));
			break;
	}
}
// 1.from replica to primary
void AsynServer::handleRepWrite(InfoForNetNode from, repType rt, opeIdType opeId) {
	string repStr;
	switch (rt) {
		case repType::replicaJounralWrite:
			repStr = "replicaJounralWrite";
			break;
		case repType::replicaDiskWrite:
			repStr = "replicaDiskWrite";
			break;
		case repType::primaryJournalWrite:
			repStr = "primaryJournalWrite";
			break;
		case repType::primaryDiskWrite:
			repStr = "primaryDiskWrite";
			break;
		default:
			LOG_ERROR("asynserver", fmt::format("[svr:[{}]] handle unexpected response of write request from [cli:[{}]]",
				info.GetConnectionstr(), from.GetConnectionstr()));
			break;
	}
	{
		LOG_ERROR("asynserver", fmt::format("[svr:[{}]] handle {} request from [cli:[{}]]",
			info.GetConnectionstr(),repStr, from.GetConnectionstr()));
	}
}

void AsynServer::handleRepRead(InfoForNetNode from, repType rt, ROPE_Result result) {
}

//@emergency0
bool AsynServer::setup_asyn_server_srv(httplib::Server* srv) {
	if (!srv)
		return false;
	else {
		try {
			auto pthis = this;
			srv->Post("/reqWrite", [pthis = pthis](const httplib::Request& req, httplib::Response& rep) {
				buffer buf(req.body);
				//any data come from http_send all should be unpack like this
				auto _from = Read<InfoForNetNode>(buf);
				auto _to = Read<InfoForNetNode>(buf);
				auto http_send_data = Read<buffer>(buf);

				auto from = Read<InfoForNetNode>(http_send_data);
				auto tos = Read<vector<InfoForNetNode>>(http_send_data);
				auto wope = Read<WOPE>(http_send_data);
				auto rt = Read<reqType>(http_send_data);
				auto opeId = Read<opeIdType>(http_send_data);
				//@dataflow opelog create
				WOpeLog opelog(opeId, wope, from, WOpeLog::wope_state_Type::init);
				pthis->fs.add_wope(move(opelog));
				});
			srv->Post("/reqRead", [pthis = pthis](const httplib::Request& req, httplib::Response& rep) {
				buffer buf(req.body);
				//any data come from http_send all should be unpack like this
				auto _from = Read<InfoForNetNode>(buf);
				auto _to = Read<InfoForNetNode>(buf);
				auto http_send_data = Read<buffer>(buf);

				auto from = Read<InfoForNetNode>(http_send_data);
				auto tos = Read<vector<InfoForNetNode>>(http_send_data);
				auto rope = Read<ROPE>(http_send_data);
				auto rt = Read<reqType>(http_send_data);
				auto opeId = Read<opeIdType>(http_send_data);
				pthis->fs.add_rope(ROpeLog(opeId, rope, from));
				});
			return true;
		}
		catch (std::exception& e) {
			LOG_ERROR("asyn", fmt::format("catch error [{}]", e.what()));
			return false;
		}
	}
}
