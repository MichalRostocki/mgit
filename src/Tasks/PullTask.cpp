#include "PullTask.h"

#include "Config.h"
#include "RepoOrchestrator.h"

namespace
{
	constexpr auto PullCommand = "git pull";
	const std::string PullLocalCommand = "git pull local ";
}

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

LocalPullTask::LocalPullTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	CommandTask(repo_orchestrator, step, PullLocalCommand + repo_orchestrator->GetConfig().default_branch)
{
}

std::string_view LocalPullTask::GetCommand()
{
	return "Pulling local...";
}
