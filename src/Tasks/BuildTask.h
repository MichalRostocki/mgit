#pragma once

#include <map>
#include <sstream>

#include "Task.h"

class BuildTask final : public Task
{
protected:
	enum class TaskStatus;

public:
	const TaskStatus* GetBuildCompleteNotifier(const std::string& required_repo) const;

	bool IsSuccessful() override;

protected:
	std::unique_ptr<TaskRunner> RegisterRepo(const RepoConfig& repository) override;
	size_t Display(std::ostream& output_stream) override;
	void OnAllReposRegistered() override;
	bool ShouldExit() override;

	enum class TaskStatus
	{
		Ongoing,
		Waiting,
		Finished,
		Failed,
		NotRun
	};

private:

	struct BuildTaskData
	{
		TaskStatus status = TaskStatus::Ongoing;

		const RepoConfig* repo_config = nullptr;
		std::string stage;
		std::stringstream last_app_output;
	};

	class BuildTaskRunner final : public TaskRunner
	{
	public:
		BuildTaskRunner(const RepoConfig& repository, const BuildTask* engine, BuildTaskData* data);

		void Run() override;

	private:
		BuildTaskData* data;
		const BuildTask* engine;
	};

	std::map<std::string, BuildTaskData> data;
	size_t max_name_space = 0;
	size_t last_max_line = 0;
	std::chrono::system_clock::time_point start_point;
};
