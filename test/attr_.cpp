#include<gtest/gtest.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<string>
//abonedoned because gcc doesn't support
//#include<format>
#include<concepts>
#include<iostream>
#include<stdio.h>
#include<filesystem>
using namespace std;
TEST(file,status) {
	;
	auto test = [](std::string path) {
		return filesystem::status(path);
	};
	auto f = test(R"(E:\GAMES\HTPlatform\HTPlatformBeta\Config.ini)");
	auto l = test(R"(D:\lkc.lnk)");
	auto fd = test(R"(C:\Windows)");
	auto non = test(R"(C:\ssss)");

}
TEST(pg, get_parent) {
	int seed = 0b00111;
	auto ct = [](int x) {
		//cout << std::format("{}: \nclz {}\ncbits{}\n", x);
	};
}