#include<test_head.h>
#include<GDFileStore.h>
#include<gtest/gtest.h>
#include<gtest/gtest.h>

TEST(Page_group, functions) {
	PageGroup pg0, pg1("pg");
	EXPECT_EQ(is_default(pg0), true);
	EXPECT_EQ(is_default(pg1), false);
}