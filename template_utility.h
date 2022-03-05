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
//template<typename SpreadNode, typename ChildTask, typename CollectRet>
//	requires invocable<ChildTask, SpreadNode>&& invocable< CollectRet, vector<invoke_result_t<ChildTask, SpreadNode>>&&>
//invoke_result_t<CollectRet, vector<invoke_result_t<ChildTask, SpreadNode>>&&>
//SpreadCall(ChildTask ct, CollectRet cr, vector<SpreadNode> vs) {
//	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode>;
//	vector<ChildTaskCallResultType> retparam;
//	for (auto& node : vs)
//		retparam.push_back(ct(node));
//	auto ret = cr(retparam);
//	return ret;
//}
//template<typename SpreadNode, invocable ChildTask, invocable CollectRet, typename ...RestParam>
//invoke_result_t<CollectRet, vector<invoke_result_t<ChildTask, SpreadNode, RestParam...>>&&>
//SpreadCall(ChildTask ct, CollectRet cr, vector<SpreadNode> vs, RestParam&&...  rp) {
//	using ChildTaskCallResultType = invoke_result_t<ChildTask, SpreadNode, RestParam...>;
//	vector<ChildTaskCallResultType> retparam;
//	for (auto& node : vs)
//		retparam.push_back(ct(node, std::forward<RestParam&&>(rp)...));
//	auto ret = cr(retparam);
//	return ret;
//}
//template<typename SpreadNode,typename ChildTask,typename CollectRet ,typename ...RestParam>
//decltype(declval<CollectRet>()(const vector<declval<ChildTask>()(SpreadNode,RestParam...&&)>() )>())
//SpreadCall(const vector<SpreadNode>& vs, RestParam... && rp) {
//	
//}
