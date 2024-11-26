#pragma once
#include "Task.h"

class CleanupTask : public Task
{
public:
	explicit CleanupTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	bool Run() override;
	std::string_view GetCommand() override;
};

class TargetedCleanupTask final : public CleanupTask
{
public:
	explicit TargetedCleanupTask(RepoOrchestrator* repo_orchestrator, StepData& step, const RepoConfig& repo_config);

protected:
	const RepoConfig& GetConfig() const override;

private:
	const RepoConfig& targeted_config;
};
