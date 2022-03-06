#include<gtest/gtest.h>
#include<GDFileStore.h>
#include<iostream>
TEST(file,read){
    auto readFile=[](const string path){
        fstream in(path);
		in.seekg(0, ios_base::end);
		size_t length = in.tellg();
		in.seekg(ios_base::beg);
		string ret;
		//the last char on file[length-1] is '\0'  on windows
		ret.resize(length);
		in.read(const_cast<char*>(ret.c_str()), length);
		return ret;
    };
    //system("echo %cd% ");
    ////on linux 3904
    //cout<<readFile("../GDFileStore.h").length()<<endl;
}