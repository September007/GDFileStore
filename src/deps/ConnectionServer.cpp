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
		auto result = fs->HandleWriteOperation(ope, osds);
		if (result == true) {
			res.body = "success";
		}
		return true;
		});
	srv.Post("/read", [](const Request& req, Response& rep) {

		});
	return true;
}
