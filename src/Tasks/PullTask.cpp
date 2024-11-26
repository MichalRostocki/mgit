#include "PullTask.h"

#include <ostream>

#include "Config.h"
#include "GitLibLock.h"
#include "Data/Data.h"

PullTask::PullTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	Task(repo_orchestrator, step)
{
}

bool PullTask::Run()
{
	GitLibLock git;
	const auto& config = GetConfig();

	if (!git.OpenRepo(config.path))
	{
		step_data.error = "Couldn't find repository";
		return false;
	}
	step_data.output << "Repository " << config.repo_name << " found" << std::endl;

	if (!git.Pull())
	{
		step_data.error = "Failed to pull";
		return false;
	}

	return true;
}

std::string_view PullTask::GetCommand()
{
	return "Pulling...";
}

TargetedPullTask::TargetedPullTask(RepoOrchestrator* repo_orchestrator, StepData& step, const RepoConfig& repo_config) :
	PullTask(repo_orchestrator, step),
	targeted_config(repo_config)
{
}

const RepoConfig& TargetedPullTask::GetConfig() const
{
	return targeted_config;
}
