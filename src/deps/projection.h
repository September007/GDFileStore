#pragma once
#include<object.h>
#include<referedBlock.h>
template<typename T>
using Attr_Type = void;
class GHObj_Attr {
public:
	ObjectWithRB rb;

	auto GetES() { return make_tuple(&rb); }
};
//template<>
//using Attr_Type<GHObject_t>= GHObj_Attr;