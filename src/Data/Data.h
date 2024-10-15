#pragma once
#include <atomic>
#include <memory>
#include <sstream>
#include <string>

class Task;

struct RepositoryInformation
{
	std::string current_branch;

	size_t sub_repo_level;
	size_t files_added = 0;
	size_t files_modified = 0;
	size_t files_deleted = 0;

	std::atomic<bool> is_repo_found{ true };
	std::atomic<bool> is_repo_detached{ false };
	std::atomic<bool> no_of_files_complete{ false };

	explicit RepositoryInformation(size_t sub_repo_level);
};

struct StepData
{
	const size_t id;
	std::unique_ptr<Task> task;

	std::atomic<bool> initialized{ false };
	std::atomic<bool> completed{ false };

	std::string error;
	std::stringstream output;
};
