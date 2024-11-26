#include "CheckoutTask.h"

#include <ostream>

#include "Config.h"
#include "GitLibLock.h"
#include "Data/Data.h"

CheckoutTask::CheckoutTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	Task(repo_orchestrator, step)
{
}

bool CheckoutTask::Run()
{
	GitLibLock git;
	const auto& config = GetConfig();

	if (!git.OpenRepo(config.path))
	{
		step_data.error = "Couldn't find repository";
		return false;
	}
	step_data.output << "Repository " << config.repo_name << " found" << std::endl;

	if(!git.FullCheckoutToIndex())
	{
		step_data.error = "Failed to checkout";
		return false;
	}

	return true;
}

std::string_view CheckoutTask::GetCommand()
{
	return "Checkout";
}

TargetedCheckoutTask::TargetedCheckoutTask(RepoOrchestrator* repo_orchestrator, StepData& step,
	const RepoConfig& repo_config) :
		CheckoutTask(repo_orchestrator, step),
		targeted_config(repo_config)
{
}

const RepoConfig& TargetedCheckoutTask::GetConfig() const
{
	return targeted_config;
}
