#pragma once
#ifndef GD_CONFIG_HEAD
#define GD_CONFIG_HEAD
#include<nlohmann/json.hpp>
#include<assistant_utility.h>
#include<map>
using nlohmann::json;
using fmt::format;

inline json GetConfigFromFile(const string& path) {
	json ret;
	try {
		auto f = ReadFile(path);
		ret = json::parse(f);
	}
	catch (std::exception& e) {
		LOG_INFO("IO", fmt::format("read config file [{}]catch exception[{}]", path,e.what()));
	}
	return ret;
}
/*
* need check empty before use
* integrated.default.json.{name}		first
* integrated.json.{name}				overload
* {name}.default.json.{name}			overload
* {name}.json.{name}					overload
* but this overwrite is over simple ,on in the surface of dirct key of  name
*/
inline json GetConfig(const string& name, const string& key, const string& default_class = "", bool reload = false) {
	//@FATAL: change root
	static string configRoot = filesystem::absolute("./../../../src/config").generic_string();
	static auto  sid = GetConfigFromFile(fmt::format("{}/integrated.default.json", configRoot));
	static auto  si = GetConfigFromFile(fmt::format("{}/integrated.json", configRoot));
	static map<string, json> cache;
	json ret;//
	string indexs[] = {
		format("integrated.default.{}", name),
		format("integrated.{}", name),
		format("{}.default", name),
		format("{}", name)
	};
	if (cache[format("integrated.default.{}", name)].empty() || reload) {
		cache[format("integrated.default.{}", name)] = GetConfigFromFile(format("{}/integrated.default.json", configRoot))[name];
	}
	if (cache[format("integrated.{}", name)].empty() || reload) {
		cache[format("integrated.{}", name)] = GetConfigFromFile(format("{}/integrated.json", configRoot))[name];
	}
	if (cache[format("{}.default", name)].empty() || reload) {
		cache[format("{}.default", name)] = GetConfigFromFile(format("{}/{}.default.json", configRoot, name));
	}
	if (cache[format("{}", name)].empty() || reload) {
		cache[format("{}", name)] = GetConfigFromFile(format("{}/{}.json", configRoot, name));
	}
	for (auto index : indexs)
		// tedious longy
		if (!(cache[index])[key].empty()/*id.find(name) != id.end() && id.find(name)->find(key) != id.find(name)->end()*/) {
			ret = (cache[index])[key];
		}
	//read default_class
	if (ret.empty()) {
		LOG_WARN("config", fmt::format("get [{}:{}],default:[{}] failed,may try default", name, key, default_class)
		,true);
		if (default_class != "") {
			ret = GetConfig(default_class, key, "", reload);
			if (ret.empty()) {
				LOG_WARN("config", fmt::format("get [{}:{}],default:[{}] failed,try default failed too",
					name, key, default_class), true);
			}
		}
	}
	return ret;
}
// use this replace directly GetConfig
template<typename T>
inline T GetconfigOverWrite(T default_value, const string& default_class, const string& name, const string& key, bool reload = false) {
	auto r = GetConfig(name, key, default_class, reload);
	if (r.empty()) {
		LOG_ERROR("config", fmt::format("can't read the [{}:{}:{}] ,use default {}, but this is not allowd in realtime",
			default_class, name, key, default_value));
		return default_value;
	}
	else {
		try {
			return r.get<T>();
		}
		catch (std::exception& e) {
			LOG_ERROR("config", fmt::format("catch error[{}],when getconfig of [{}:{}:{}]",
				e.what(), default_class, name, key));
			return default_value;
		}
	}
}
#endif //GD_CONFIG_HEAD