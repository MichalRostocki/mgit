#pragma once
#include <list>
#include <thread>

struct RepoConfig;
struct Config;
class RepositoryController;

class Task
{
	class TaskRunner;

public:
	Task(const Task& other) = delete;
	Task(Task&& other) noexcept = delete;
	Task& operator=(const Task& other) = delete;
	Task& operator=(Task&& other) noexcept = delete;

	Task() = default;
	virtual ~Task();

	virtual void Register(const Config& config);
	virtual void Process(std::ostream& output_stream);

protected:
	class TaskRunner
	{
	public:
		explicit TaskRunner(const RepoConfig& repo_config);
		virtual ~TaskRunner() = default;

		TaskRunner(const TaskRunner& other) = delete;
		TaskRunner(TaskRunner&& other) noexcept = delete;
		TaskRunner& operator=(const TaskRunner& other) = delete;
		TaskRunner& operator=(TaskRunner&& other) noexcept = delete;

		virtual void Run() = 0;

		void Stop();
		std::string_view GetRepoName() const;

	protected:
		const RepoConfig& repo_config;
		const std::string repo_name;

		bool should_stop = false;
	};

	std::list<std::unique_ptr<TaskRunner>> runners;
	std::list<std::thread> threads;

	static void ClearCurrentLine(std::ostream& output_stream);


	virtual std::unique_ptr<TaskRunner> RegisterRepo(const RepoConfig& repository) = 0;
	// Should return numbers of lines written
	virtual size_t Display(std::ostream& output_stream) = 0;
	virtual void OnAllReposRegistered() = 0;
	virtual bool ShouldExit() = 0;
	virtual bool IncludesHidden() = 0;

private:
	void StopProcedure();
};

#define TASK_RUNNER_CHECK {if(should_stop)return;}
