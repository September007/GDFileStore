#include<GDFileStore.h>
#include<gtest/gtest.h>
#include<filesystem>
#include<string_view>
#include<random>
#include<chrono>
namespace fs   =std::filesystem;
struct GDFSTest {
	static void GDFS_Store_Reserve() {
		std::mt19937 rando(chrono::system_clock::now().time_since_epoch().count());
		string  path = "/testFolder";
		GDFileStore g(path);
		vector<string> names, contents;
		constexpr int TC = 100;
		for (int i = 0; i < TC; i++) {
			names.push_back(fmt::format("test{:-03d}" , i));
			int len = (rando() % 10) + 1;//+1 for set postive
			string content = "meaningless content:\n";
			content.reserve(len + 20);
			while (--len > 0) {
				content.append(1, char(rando() % 60 + '0'));
			}
			contents.push_back(std::move(content));
		}
		for (int i = 0; i < TC; ++i) {
			auto ret = g._StoreObjectData(Object{ names[i] }, contents[i]);
			EXPECT_EQ(ret, true);
		}
		for (int i = 0; i < TC; ++i) {
			auto readContent = g._get_object_data(Object{ names[i] });
			auto con=contents[i];
			auto len0=readContent.length(),len1=con.length();
			EXPECT_EQ(readContent, contents[i]);
		}
		//std::filesystem::remove_all(path);
	}
};
 TEST(GDFS, Store_Reserve) {
	 GDFSTest::GDFS_Store_Reserve();
 }
TEST(GDFS, file_stat) {

}