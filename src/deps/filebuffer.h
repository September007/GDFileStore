#pragma once
#include<assistant_utility.h>
#include<set>
#include<shared_mutex>
#include<object.h>
using namespace std;
//the represent a file span
class filecache;
class filespan_data;
class filespan;

class filecache {
	map<string, string> cache;
	set<string> files;
	vector<string> need_load_files;
	//should recursive?
	std::shared_mutex access_to_cache;
	std::shared_mutex access_to_files;
public:
	void register_file(const string& filename) {
		std::unique_lock lg(access_to_files);
		auto x = files.insert(filename);
		if (x.second)
			need_load_files.push_back(filename);
	}
	void flush_cache() {
		std::unique_lock lgf(access_to_files), lgc(access_to_cache);
		for(auto &f:need_load_files)
			if (cache.find(f) == cache.end()) {
				auto fc = ReadFile(f);
				cache.emplace(make_pair(move(f), move(fc)));
			}
		files.clear();
	}
	//return the count of files that need to be load
	int floatingFilesCount() {
		std::shared_lock lg(access_to_files);
		return need_load_files.size();
	}
	//call flush_cache first
	string GetFile(string filepath) {
		//check hanging
		DebugArea(LOG_EXPECT_EQ("filebuffer", floatingFilesCount(), 0));
		std::shared_lock lgc(access_to_cache);
		auto x = cache.find(filepath);
		DebugArea(LOG_EXPECT_TRUE("filebuffer", x != cache.end()));
		return x->second;
	}
};
class filespan_data {
	// the data source file
	string str;
	//the offset of the span should be on the file
	int offset;
	int length;
	//if true,
	bool is_str_data_not_filepath;
	filespan_data(bool is_str_data_not_filepath, const string& str, int offset, int length)
		:is_str_data_not_filepath(is_str_data_not_filepath), str(str), offset(offset), length(length) {
	}
	void registerFile(filecache& fc) { fc.register_file(str); }
	string Get(filecache& fc) {
		if (is_str_data_not_filepath)return str;
		else return fc.GetFile(str);
	}
};
class filespan {
	//the span 
	int length;
	filespan_data	data;
public: 
	template<typename ...args>
	filespan(int length, args...ags) :length(length), data(forward<args>(ags)...) {}
	int GetSpanLength() { return length; }

};

class filebuffer {
public:
	filecache fc;
	list<filespan> fss;
	int filelength = 0;
	void add(Operation wope) {
		switch (wope.operationType) {
			case OperationType::Insert:
			{
				
			}
			default:
				break;
		}
	}
};