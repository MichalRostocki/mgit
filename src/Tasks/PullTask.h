#pragma once
#include "Task.h"

struct RepoConfig;

class PullTask : public Task
{
public:
	explicit PullTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	bool Run() override;
	std::string_view GetCommand() override;
};

class TargetedPullTask final : public PullTask
{
public:
	explicit TargetedPullTask(RepoOrchestrator* repo_orchestrator, StepData& step, const RepoConfig& repo_config);

protected:
	const RepoConfig& GetConfig() const override;

private:
	const RepoConfig& targeted_config;
};
