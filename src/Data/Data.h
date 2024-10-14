#pragma once
#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct CommandData;
class ControllerNode;
class Task;
struct RepoConfig;

struct RepoCommandData
{
	const RepoConfig& repo_config;
	std::set<std::string> await_list;
	std::vector<std::unique_ptr<CommandData>> steps_data;

	explicit RepoCommandData(const RepoConfig& repo_config);

	const CommandData* GetActiveOrLast() const;
	const CommandData* GetActive() const;

	void SkipToTask(size_t size);
};

struct RepoTaskCollection
{
	std::vector<std::unique_ptr<Task>> tasks;
	std::atomic<size_t> current_task_index{ 0 };
	std::atomic<bool> error_encountered{ false };

	std::shared_ptr<ControllerNode> GetFinishNotifier();
	std::shared_ptr<ControllerNode> GetErrorNotifier();
	bool IsComplete() const;
	void Stop();
};
