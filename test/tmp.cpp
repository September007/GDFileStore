#include<test_head.h>
#include<filebuffer.h>
#include<assistant_utility.h>
#define head TMP
#include<vector>
using namespace std;
TEST(filebuffer, all) {
	class x {
	public:
		void add() {
			std::cout << "ss" << endl;
		}
	};
	x xx;
	thread t(&x::add, &xx);
	t.join();
}

