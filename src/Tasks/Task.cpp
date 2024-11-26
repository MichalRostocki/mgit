#include "Task.h"

#include "RepoOrchestrator.h"

Task::Task(RepoOrchestrator* parent, StepData& step_data) :
	step_data(step_data),
	parent(parent)
{
}

Task::~Task() = default;

void Task::Stop()
{
	should_stop = true;
}

const RepoConfig& Task::GetConfig() const
{
	return parent->GetConfig();
}

RepositoryInformation& Task::GetRepositoryInformation() const
{
	return parent->GetRepositoryInfo();
}
