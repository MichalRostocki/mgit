#include "RepoOrchestrator.h"

#include "Config.h"
#include "Tasks/CheckoutTask.h"
#include "Tasks/CleanupTask.h"
#include "Tasks/CommandTask.h"
#include "Tasks/PullPrepareTask.h"
#include "Tasks/PullTask.h"
#include "Tasks/StatusTask.h"

RepoOrchestrator::RepoOrchestrator(const RepoConfig& repo_config, const size_t sub_repo_level) :
	repo_config(repo_config),
	info(sub_repo_level)
{
}

void RepoOrchestrator::Launch()
{
	if(!running_thread.joinable())
	{
		running_thread = std::jthread([this] {InternalRun(); });
	}
}

void RepoOrchestrator::Stop()
{
	should_stop = true;
	for (const auto & step_data : steps)
		step_data->task->Stop();

	if(running_thread.joinable())
		running_thread.join();
}

OrchestratorStatus RepoOrchestrator::GetCurrentStatus() const
{
	if (error_encountered)
		return OrchestratorStatus::Error;

	if (!await_list.empty())
		return OrchestratorStatus::Awaiting;

	if (IsComplete())
		return OrchestratorStatus::Complete;

	return OrchestratorStatus::Ongoing;
}

bool RepoOrchestrator::HasError() const
{
	return error_encountered;
}

bool RepoOrchestrator::IsComplete() const
{
	return current_task_index > last_task;
}

void RepoOrchestrator::RegisterChild(const std::shared_ptr<RepoOrchestrator>& child)
{
	children.insert(child);
}

const std::set<std::shared_ptr<RepoOrchestrator>>& RepoOrchestrator::GetChildren() const 
{
	return children;
}

void RepoOrchestrator::RegisterListener(RepoOrchestrator* notified)
{
	registered_to_notify.insert(notified);
}

void RepoOrchestrator::Notify(const std::string& notifier)
{
	mutex.lock();
	await_list.erase(notifier);
	mutex.unlock();
}

void RepoOrchestrator::PlanStatusJob()
{
	auto step_data = std::make_shared<StepData>(steps.size());
	step_data->task = std::make_unique<StatusTask>(this, *step_data);
	steps.push_back(std::move(step_data));
}

void RepoOrchestrator::PlanPullPrepareJob()
{
	auto step_data = std::make_shared<StepData>(steps.size());
	step_data->task = std::make_unique<PullPrepareTask>(this, *step_data);
	steps.push_back(std::move(step_data));
}

void RepoOrchestrator::PlanBuildJobs()
{
	PlanBuildJobs(repo_config.build.steps);
	last_task = static_cast<int64_t>(steps.size()) - 1;

	if(repo_config.build.on_error.retry)
	{
		PlanBuildJobs(repo_config.build.on_error.before_retry);
		PlanBuildJobs(repo_config.build.steps);
	}

	CreateAwaitList();
}

void RepoOrchestrator::PlanPullJob()
{
	auto step_data = std::make_shared<StepData>(steps.size());
	step_data->task = std::make_unique<PullTask>(this, *step_data);
	steps.push_back(std::move(step_data));
}

void RepoOrchestrator::PlanCheckoutPullJob()
{
	auto checkout_step_data = std::make_shared<StepData>(steps.size());
	checkout_step_data->task = std::make_unique<CheckoutTask>(this, *checkout_step_data);
	steps.push_back(std::move(checkout_step_data));

	auto cleanup_step_data = std::make_shared<StepData>(steps.size());
	cleanup_step_data->task = std::make_unique<CleanupTask>(this, *cleanup_step_data);
	steps.push_back(std::move(cleanup_step_data));

	auto pull_step_data = std::make_shared<StepData>(steps.size());
	pull_step_data->task = std::make_unique<PullTask>(this, *pull_step_data);
	steps.push_back(std::move(pull_step_data));

	for (const auto& child : repo_config.sub_repos)
		PlanCheckoutPullJob(child);

	last_task = static_cast<int64_t>(steps.size()) - 1;
}

void RepoOrchestrator::ClearSteps()
{
	Stop();

	last_task = 0;
	current_task_index = -1;
	error_encountered = false;
	should_stop = false;
	steps.clear();
}

const RepoConfig& RepoOrchestrator::GetConfig() const
{
	return repo_config;
}

RepositoryInformation& RepoOrchestrator::GetRepositoryInfo()
{
	return info;
}

std::string_view RepoOrchestrator::GetAwait() const
{
	if (await_list.empty())
		return {};
	return *await_list.begin();
}

int64_t RepoOrchestrator::GetActiveId() const
{
	return current_task_index;
}

size_t RepoOrchestrator::GetSize() const
{
	return steps.size();
}

std::string_view RepoOrchestrator::GetActiveCommand() const
{
	const int64_t index = current_task_index;
	if (index == -1 || index >= static_cast<int64_t>(steps.size()))
		return {};
	return steps[index]->task->GetCommand();
}

std::string_view RepoOrchestrator::GetErrorString() const
{
	const int64_t index = current_task_index;
	if (index == -1 || index >= static_cast<int64_t>(steps.size()))
		return {};
	return steps[index]->error;
}

std::string RepoOrchestrator::GetActiveOutput() const
{
	const int64_t index = current_task_index;
	if (index == -1 || index >= static_cast<int64_t>(steps.size()))
		return {};
	return steps[index]->output.str();
}

void RepoOrchestrator::PlanCheckoutPullJob(const RepoConfig& config)
{
	auto checkout_step_data = std::make_shared<StepData>(steps.size());
	checkout_step_data->task = std::make_unique<TargetedCheckoutTask>(this, *checkout_step_data, config);
	steps.push_back(std::move(checkout_step_data));

	auto cleanup_step_data = std::make_shared<StepData>(steps.size());
	cleanup_step_data->task = std::make_unique<TargetedCleanupTask>(this, *cleanup_step_data, config);
	steps.push_back(std::move(cleanup_step_data));

	auto pull_step_data = std::make_shared<StepData>(steps.size());
	pull_step_data->task = std::make_unique<TargetedPullTask>(this, *pull_step_data, config);
	steps.push_back(std::move(pull_step_data));

	for (const auto& child : config.sub_repos)
		PlanCheckoutPullJob(child);
}

void RepoOrchestrator::PlanBuildJobs(const std::vector<std::string>& jobs)
{
	for (const auto& command : jobs)
	{
		auto step = std::make_shared<StepData>(steps.size());
		step->task = std::make_unique<CommandTask>(this, *step, command);
		steps.push_back(std::move(step));
	}
}

void RepoOrchestrator::CreateAwaitList()
{
	await_list.insert_range(repo_config.build.require);
}

void RepoOrchestrator::HandleError()
{
	if(last_task != static_cast<int64_t>(steps.size() - 1))
	{
		// If last task is not the last one - retry procedure is enabled
		current_task_index = last_task + 1;
	}
	else
	{
		error_encountered = true;
		should_stop = true;
	}
}

void RepoOrchestrator::InternalRun()
{
	// Sleep until await list is empty
	while (!await_list.empty())
	{
		if (should_stop)
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
	}
	if (should_stop)
		return;

	current_task_index = 0;
	while(current_task_index <= last_task)
	{
		const auto& current_step = steps[current_task_index];

		current_step->initialized = true;
		const bool result = current_step->task->Run();
		current_step->completed = true;

		if(!result)
			HandleError();
		else
			++current_task_index;

		if (should_stop)
			return;
	}

	for (auto* to_notify : registered_to_notify)
		to_notify->Notify(repo_config.repo_name);
}
