#include "Task.h"

#include "Config.h"
#include "Nodes/SimpleTaskControllerNode.h"

Task::Task(const RepoConfig& repo_config): repo_config(repo_config)
{
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Task::Launch()
{
	if(!running_thread.joinable())
		running_thread = std::thread{ [this] {this->InternalRun(); } };
}

Task::~Task()
{
	should_stop = true;
	if (running_thread.joinable())
		running_thread.join();
}

void Task::NotifyOnEnd(const std::shared_ptr<ControllerNode>& node)
{
	attached_nodes.push_back(node);
}

void Task::NotifyOnError(const std::shared_ptr<ControllerNode>& node)
{
	error_nodes.push_back(node);
}

void Task::ForceStop()
{
	should_stop = true;
}

std::shared_ptr<ControllerNode> Task::GetSimpleNotifier()
{
	return std::make_shared<SimpleTaskControllerNode>(this);
}

void Task::InternalRun()
{
	try
	{
		Run();
	}
	catch(...)
	{
		error_encountered = true;
	}

	if (!should_stop)
	{
		if (error_encountered)
			NotifyNodes(error_nodes);
		else NotifyNodes(attached_nodes);
	}

	attached_nodes.clear();
	error_nodes.clear();
	is_complete = true;
}

void Task::NotifyNodes(const std::list<std::shared_ptr<ControllerNode>>& nodes)
{
	for (const auto & controller_node : nodes)
	{
		controller_node->Notify(repo_config.repo_name);
	}
}
