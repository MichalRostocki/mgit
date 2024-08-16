#pragma once

#include "Task.h"

class StatusTask final : public Task
{
protected:
	std::unique_ptr<TaskRunner> RegisterRepo(const RepoConfig& repository) override;
	void OnAllReposRegistered() override;
	size_t Display(std::ostream& output_stream) override;
	bool ShouldExit() override;
	bool IncludesHidden() override;

private:
	struct StatusRepoData
	{
		bool is_complete = false;
		bool error_encountered = false;

		bool is_repo_found = true;
		bool is_repo_detached = false;

		bool no_of_files_started = false;
		bool no_of_files_complete = false;

		std::string repo_name;
		std::string default_branch;
		std::string current_branch;

		size_t files_added = 0;
		size_t files_modified = 0;
		size_t files_deleted = 0;
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
