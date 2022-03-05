#include<type_traits>
#include<concepts>
#include<vector>
using namespace std;

template<typename T1,typename T2>
auto max(T1 l, T2 r) {
	return l > r ? l : r;
}
//distribute task
template<typename SpreadNode, typename ChildTask, typename CollectRets,typename ...RestParams>
	requires invocable<ChildTask, SpreadNode,RestParams...>
		&&	 invocable< CollectRets, vector<invoke_result_t<ChildTask, SpreadNode,RestParams...>>&&>
invoke_result_t<CollectRets, vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>&&>
SpreadCall(ChildTask ct, CollectRets cr, vector<SpreadNode> vs,RestParams&&... restParams) {
	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode, RestParams...>;
	vector<ChildTaskCallResultType> retparam;
	for (auto& node : vs)
		retparam.push_back(ct(node,forward<RestParams>(restParams)...));
	auto ret = cr(retparam);
	return ret;
}

template<typename SpreadNode, typename ChildTask,typename ...RestParams>
	requires invocable<ChildTask, SpreadNode,RestParams...>
vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>>
SpreadCall(ChildTask ct, vector<SpreadNode> vs,RestParams&&... restParams) {
	return SpreadCall(ct,
	[](vector<invoke_result_t<ChildTask, SpreadNode, RestParams...>> rets){return rets;},
		vs,	forward<RestParams>(restParams)...);
}
