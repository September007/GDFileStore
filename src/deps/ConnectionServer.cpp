#include<ConnectionServer.h>

void FSConnnectionServer::listen(GDFileStore* fs)   try {
	if (srv.is_running())
		srv.stop();
	if (!this->srv.is_valid()) {
		LOG_ERROR("server", "server is invalid");
	};
	auto http_config = GetConfig(fs->GetConfigName(), "http_server");
	auto host = http_config["addr"].get<string>();
	auto port = http_config["port"].get<int>();
	if (host == "" || port == int{}) {
		LOG_ERROR("server", "read server config failed");
		return;
	}
	thread listenThread{ SrvThreadMain,fs,&this->srv };
	//free this
	listenThread.detach();
}
catch (std::exception& e) {
	LOG_ERROR("server", fmt::format("server::listen get error[{}]", e.what()));
	return;
}
void FSConnnectionServer::stop() {
	//dont konw if this is thread-safe
	srv.stop();
}
void FSConnnectionServer::SrvThreadMain(GDFileStore* fs, httplib::Server* p_srv) {
	if (!p_srv) {
		LOG_ERROR("sever", fmt::format("get null server"));
		return;
	}
	// refer
	auto& srv = *p_srv;
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
		return true;
		});

}