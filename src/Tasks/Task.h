#pragma once
#include <atomic>
#include <list>
#include <thread>

#include "Nodes/ControllerNode.h"

struct RepoConfig;

class Task
{
public:
	explicit Task(const RepoConfig& repo_config);
	virtual ~Task();

	void Launch();
	void NotifyOnEnd(const std::shared_ptr<ControllerNode>& node);
	void NotifyOnError(const std::shared_ptr<ControllerNode>& node);
	void ForceStop();

	std::shared_ptr<ControllerNode> GetSimpleNotifier();
	void InvokeSuccess();

	Task(const Task& other) = delete;
	Task(Task&& other) noexcept = delete;
	Task& operator=(const Task& other) = delete;
	Task& operator=(Task&& other) noexcept = delete;

protected:
	const RepoConfig& repo_config;

	std::thread running_thread;
	std::list<std::shared_ptr<ControllerNode>> attached_nodes;
	std::list<std::shared_ptr<ControllerNode>> error_nodes;

	std::atomic<bool> should_stop = false;
	std::atomic<bool> error_encountered = false;

private:
	void InternalRun();
	virtual void Run() = 0;

	std::atomic<bool> is_complete = false;

	void NotifyNodes(const std::list<std::shared_ptr<ControllerNode>>& nodes);
};

#define TASK_RUNNER_CHECK if(should_stop)return
