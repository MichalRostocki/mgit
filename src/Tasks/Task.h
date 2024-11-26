#pragma once
#include <atomic>
#include <string_view>

struct RepositoryInformation;
struct RepoConfig;
struct StepData;
class RepoOrchestrator;

class Task
{
public:
	explicit Task(RepoOrchestrator* parent, StepData& step_data);
	virtual ~Task();

	Task(const Task& other) = delete;
	Task(Task&& other) noexcept = delete;
	Task& operator=(const Task& other) = delete;
	Task& operator=(Task&& other) noexcept = delete;

	virtual bool Run() = 0;
	virtual std::string_view GetCommand() = 0;

	void Stop();

protected:
	StepData& step_data;

	std::atomic<bool> should_stop;

	virtual const RepoConfig& GetConfig() const;
	virtual RepositoryInformation& GetRepositoryInformation() const;

private:
	RepoOrchestrator* parent;
};

#define TASK_RUNNER_CHECK if(should_stop)return false
