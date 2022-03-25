#include<object.h>


string GHObject_t::GetLiteralDescription(GHObject_t const& ghobj) {
	return fmt::format("{}.{}", ghobj.owner, ghobj.hobj.oid.name);
}