#pragma once
#include "Config.h"
#include "Tasks/Task.h"

class Display;
struct RepoTaskCollection;

class MultiController
{
public:
    bool LoadConfig(std::ostream& error_stream);

    int DisplayStatus();
    int Build();

    const RepoConfig* GetRepo(const std::string_view& repo_name) const;

private:
    Config config;
    std::map<std::string, std::shared_ptr<RepoTaskCollection>> tasks;

    bool ShouldExit() const;
    bool HasError() const;
    int RunTask(const std::list<Task*>& default_tasks, Display& display);
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
