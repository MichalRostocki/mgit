#include "RepoOrchestrator.h"

#include "Config.h"
#include "Tasks/CommandTask.h"
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

void RepoOrchestrator::Register(RepoOrchestrator* notified)
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
