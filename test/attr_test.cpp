#include<gtest/gtest.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<string>
#include<format>
#include<iostream>
#include<stdio.h>
using namespace std;
TEST(file,stat) {
	;
	auto test = [](std::string path) {
		struct stat *st=new struct stat;
		stat(path.c_str(),st);
		return st;
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