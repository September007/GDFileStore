#define head TMP2
#include<iostream>
#include<concepts>
#include<gtest/gtest.h>
#include<fmt/format.h>
#include<coroutine>
#include<memory>
using namespace std;
using fmt::format;
class coruntine {
public:
	class promise_type;
	using handle_type = coroutine_handle<promise_type>;
	class promise_type {
	public:
		int i;
		char  id;
		promise_type(int i, char id) :i(i), id(id) {
			cout << format("promise create with{}:{}", id,i) << endl;
		}
		auto get_return_object() {
			//return coruntine(handle_type::from_promise(*this));
		}
	};
};

TEST(head, make_sh) {
	string s = "adsaa";
	auto ss = make_shared<string>(s);
	auto mss = make_shared<string>(move(s));
}