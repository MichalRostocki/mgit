#pragma once

#include "Task.h"

class StatusTask final : public Task
{
public:
	std::unique_ptr<TaskRunner> RegisterRepo(const RepoConfig& repository) override;
	void OnAllReposRegistered() override;
	size_t Display(std::ostream& output_stream) override;
	bool ShouldExit() override;

private:
	struct StatusRepoData
	{
		bool is_complete = false;
		bool error_encountered = false;

		bool is_repo_found = true;
		bool is_repo_detached = false;
		std::string current_branch;
		std::string repo_name;
	};

	class StatusTaskRunner final : public TaskRunner
	{
	public:
		StatusTaskRunner(const RepoConfig& repo, StatusRepoData* data);

		void Run() override;

	private:
		StatusRepoData* data = nullptr;

		void GitError();
	};

	std::list<StatusRepoData> repositories_data;
	size_t max_name_space = 0;
};
