#include "CleanupTask.h"

CleanupTask::CleanupTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	Task(repo_orchestrator, step)
{
}

bool CleanupTask::Run()
{
	return true;
}

std::string_view CleanupTask::GetCommand()
{
	return "Cleanup";
}

TargetedCleanupTask::TargetedCleanupTask(RepoOrchestrator* repo_orchestrator, StepData& step,
	const RepoConfig& repo_config) :
		CleanupTask(repo_orchestrator, step),
		targeted_config(repo_config)
{
}

const RepoConfig& TargetedCleanupTask::GetConfig() const
{
	return targeted_config;
}
