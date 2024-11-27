#include "PullTask.h"

constexpr auto PullCommand = "git pull";

PullTask::PullTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	CommandTask(repo_orchestrator, step, PullCommand)
{
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
