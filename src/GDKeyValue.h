#pragma once
#ifndef GDKEYVALUE_HEAD
#define GDKEYVALUE_HEAD
#include<rocksdb/db.h>
//kv interface

class KVBase {
public:
	virtual ~KVBase() {};

};


#endif //GDKEYVALUE_HEAD