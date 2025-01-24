#pragma once
#include "CommandTask.h"

class PullTask : public CommandTask
{
public:
	explicit PullTask(RepoOrchestrator* repo_orchestrator, StepData& step);

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

class LocalPullTask : public CommandTask
{
public:
	explicit LocalPullTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	std::string_view GetCommand() override;
};
