#pragma once
#include "Task.h"

class CheckoutTask : public Task
{
public:
	explicit CheckoutTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	bool Run() override;
	std::string_view GetCommand() override;
};

class TargetedCheckoutTask final : public CheckoutTask
{
public:
	explicit TargetedCheckoutTask(RepoOrchestrator* repo_orchestrator, StepData& step, const RepoConfig& repo_config);

protected:
	const RepoConfig& GetConfig() const override;

private:
	const RepoConfig& targeted_config;
};
