#include "CountedTaskControllerNode.h"

#include "Tasks/Task.h"

CountedTaskControllerNode::CountedTaskControllerNode(Task* task, const uint64_t expected_count) :
	task(task),
	expected_count(expected_count)
{
}

void CountedTaskControllerNode::Notify(const std::string& notify_data)
{
	if (--expected_count == 0)
		task->Launch();
}
