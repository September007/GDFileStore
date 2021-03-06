#include<assistant_utility.h>
#include<filesystem>
#include<GDMutex.h>
shared_ptr<spdlog::logger> GetLogger(const string& name, const bool force_isolate, const string& fileClass) {
	//@Todo: read log file root from json
	static auto logFileRoot = std::filesystem::absolute("./tmp/logs").string();
	// set all output into one file for debug
	//if (!force_isolate && name != "integrated")
	//	return GetLogger("integrated");
	auto ret = spdlog::get(name);
	// if missing,create
	if (ret == nullptr) {
		{
			auto logfilename = fmt::format("{}/{}.log", logFileRoot, force_isolate ? name : fileClass);
			bool create_new = false;
			ret = spdlog::synchronous_factory::create<
				spdlog::sinks::basic_file_sink_st>(name, logfilename, false);
			if (ret != nullptr)
				create_new = true;
			// if create failed, return default
			if (!ret) {
				// try return selfdefined first
				if (name != "default")
					ret = GetLogger("default", true, "default");
			}
			if (!ret) {
				spdlog::warn("create log file[{}]failed {}:{}", logfilename, __FILE__,
					__LINE__);
				ret = spdlog::default_logger();
			}
			if (create_new) {
				ret->info("log:{} just created", name);
			}
			else {
				ret->info("creating log: {} just failed, this is default", name);
			}
		}
	}
	return ret;
}

nlohmann::json GetSetting(const string& settingFile) {
	static nlohmann::json default_setting = R"({
	"setting_name":"default",
	"bool":{
		"true_value":true,
		"false_value":false
	},
	"string": "str_string"
})"_json;
	ifstream in(settingFile);
	nlohmann::json setting;
	if (in.good())
		in >> setting;
	else {
		GetLogger("json setting")
			->error("read setting file failed.{}[{}:{}]", settingFile, __FILE__,
				__LINE__);
		setting = default_setting;
	};
	return setting;
};
string ReadFile(const string& path) {
	fstream in(path);
	if (!in.good()) {
		LOG_INFO("IO", fmt::format("readfile[{}] failed.", path));
		return "";
	}
	string ret;
#ifdef __linux__
	in.seekg(0, ios_base::end);
	size_t length = in.tellg();
	in.seekg(ios_base::beg);
	ret.resize(length);
	in.read(const_cast<char*>(ret.c_str()), length);
	// if (ret.back() == '\0')ret.pop_back();
#else
	// windows file storage would append {how much '\n' in file} '\0' chars at end
	// of file so read as above would read these '\0'
#ifdef _WIN32
	stringstream ss;
	ss << in.rdbuf();
	ret = ss.str();
#endif
#endif
	return ret;
}
bool WriteFile(const string& path, const string& content, const bool create_parent_dir_if_missing) try {
	//auto& m = GetMutex(__func__);
	//unique_lock lg(m);
	auto&objDir = path;
	ofstream out(objDir);
	if (!out.good()) {
		auto parentDir = filesystem::path(objDir).parent_path();
		// maybe missing pg directory
		if (create_parent_dir_if_missing &&
			!filesystem::is_directory(parentDir.c_str())) {
			//GetLogger("IO")->warn(
			//"write file[{}] failed because parent dir missed,now creating.{}:{}",
			//	path, __FILE__, __LINE__);
			filesystem::create_directories(parentDir);
			out.open(objDir);
			LOG_ASSERT_TRUE("IO", out.good(), "write file because create parent dir failed ");
		}
		else {
			GetLogger("IO")->error("open file[{}] failed .{}:{}", path, __FILE__,
				__LINE__);
			return false;
		}
	}
	out << content << flush;
	out.close();
	return out.good();
}
catch (std::exception& ex) {
	GetLogger("IO")->error("catch error at{}:{}\n{}", __FILE__, __LINE__, ex.what());
	throw ex;
	return false;
}

void Error_Exit(const string &msg) {
	spdlog::default_logger()->error("msg:[{}]get error exit,check log for more info",msg);
	exit(0);
}

std::string getTimeStr(std::string_view fmt) {
	std::time_t now =
		std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char s[40] = {};
	std::strftime(&s[0], 40, fmt.data(), std::localtime(&now));
	return s;
}



Slice:: Slice(const string& str) :data(make_shared<buffer>(str)), start(0), end(str.size()) {
}

void Slice::Read(buffer& buf,const Slice* sli) {
	new(&const_cast<Slice*>(sli)->data)shared_ptr<buffer>(make_shared<buffer>());
	::Read(buf, &sli->data->data);
}

void Slice::Write(buffer& buf,const Slice* s) {
	if (s->data != nullptr)
		::Write(buf, &s->data->data);
	else {
		buffer b;
		::Write(buf, &b);
	}
	using tif = tuple<int*, float*>;
	tif a;
	auto i=std::get<0>(a);
}

