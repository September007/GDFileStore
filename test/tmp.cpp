#include<test_head.h>
#include<filebuffer.h>
#include<assistant_utility.h>
#define head TMP
#include<vector>
using namespace std;
TEST(filebuffer, all) {
	vector<int > vi = { 1,2,3,4 };
	int i = 5;
	double d = 6.0;
	buffer b;
	//auto tp = make_tuple(&vi, &i, &d, &b);
	//Write(b, &tp);
	MultiWrite(b, vi, i, d, b);
	auto rv = Read<vector<int>>(b);
	auto ri = Read<int>(b);
	auto rd = Read<double>(b);

	rv[0];
}

