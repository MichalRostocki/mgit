#include "SimpleControllerNode.h"

SimpleControllerNode::SimpleControllerNode(const std::function<void(const std::string&)>& function) : function(function)
{
}

void SimpleControllerNode::Notify(const std::string& notify_data)
{
	function(notify_data);
}
