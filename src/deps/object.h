
class PageGroup {
public:
	string name;
	PageGroup(const string& name = "") :name(name) {}
	PageGroup(const string&& name) :name(name) {}
	bool operator==(const PageGroup&)const = default;
	bool operator < (const PageGroup&)const = default;
};

class Object {
public:
	string name;
	Object(const string& name = "") :name(name) {}
	Object(const string&& name) :name(name) {}
	bool operator==(const Object&)const = default;
	bool operator < (const Object&)const = default;
};


