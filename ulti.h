#include<type_traits>
#include<vector>
using namespace std;
template<typename T,typename ...ParamsType>
using RetType = decltype(declval<T>(declval<ParamsType...>()));

//template<typename SpreadNode,typename ChildTask,typename CollectRet ,typename ...RestParam>
//decltype(declval<CollectRet>()(const vector<declval<ChildTask>()(SpreadNode,RestParam...&&)>() )>())
//SpreadCall(const vector<SpreadNode>& vs, RestParam... && rp) {
//	
//}
