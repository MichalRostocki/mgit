#include "Task.h"

Task::Task(RepoOrchestrator* parent, StepData& step_data) :
	parent(parent),
	step_data(step_data)
{
}

Task::~Task() = default;

void Task::Stop()
{
	should_stop = true;
}
