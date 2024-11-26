#pragma once
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "Data/Data.h"

class MultiController;
struct RepoConfig;

enum class OrchestratorStatus
{
	Awaiting,
	Ongoing,
	Complete,
	Error,
};

class RepoOrchestrator
{
public:
	RepoOrchestrator(const RepoConfig& repo_config, size_t sub_repo_level);

	void Launch();
	void Stop();

	OrchestratorStatus GetCurrentStatus() const;
	bool HasError() const;
	bool IsComplete() const;

	void RegisterListener(RepoOrchestrator* notified);
	void Notify(const std::string& notifier);

	void RegisterChild(const std::shared_ptr<RepoOrchestrator>& child);
	const std::set<std::shared_ptr<RepoOrchestrator>>& GetChildren() const;

	void PlanStatusJob();
	void PlanPullPrepareJob();
	void PlanBuildJobs();
	void PlanPullJob();
	void PlanCheckoutPullJob();
	void ClearSteps();

	const RepoConfig& GetConfig() const;
	RepositoryInformation& GetRepositoryInfo();
	std::string_view GetAwait() const;
	int64_t GetActiveId() const;
	size_t GetSize() const;
	std::string_view GetActiveCommand() const;
	std::string_view GetErrorString() const;
	std::string GetActiveOutput() const;

private:
	const RepoConfig& repo_config;
	RepositoryInformation info;
	std::set<std::shared_ptr<RepoOrchestrator>> children;

	std::mutex mutex;
	std::set<std::string> await_list;
	std::vector<std::shared_ptr<StepData>> steps;
	std::set<RepoOrchestrator*> registered_to_notify;

	// Task that is last when no errors arise
	int64_t last_task{0};

	std::atomic<int64_t> current_task_index{ -1 };
	std::atomic<bool> error_encountered{ false };

	std::atomic<bool> should_stop{ false };
	std::jthread running_thread;

	void PlanCheckoutPullJob(const RepoConfig& config);
	void PlanBuildJobs(const std::vector<std::string>& jobs);
	void CreateAwaitList();

	void HandleError();	
	void InternalRun();
};
