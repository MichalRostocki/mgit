#include "SimpleTaskControllerNode.h"

#include "Tasks/Task.h"

SimpleTaskControllerNode::SimpleTaskControllerNode(Task* task) : task(task)
{
}

void SimpleTaskControllerNode::Notify(const std::string& notify_data)
{
	task->Launch();
}
