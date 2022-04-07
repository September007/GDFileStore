#include<test_head.h>
#include<filebuffer.h>
#include<assistant_utility.h>
#define head TMP
#include<vector>
#include<connection.h>
using namespace std;
TEST(filebuffer, all) {
	WOPE wope;
	wope.block_data = "Dasdads";
	wope.block_num = 1;
	
	auto x = move(wope);
	x.block_num++;
}

