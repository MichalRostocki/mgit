#pragma once
#include "Config.h"
#include "Tasks/Task.h"
#include "OrderedMap.h"

class Display;
class RepoOrchestrator;

using MultiControllerTasks = OrderedMap<std::string, std::shared_ptr<RepoOrchestrator>>;

class MultiController
{
public:
    bool LoadConfig(std::ostream& error_stream);

    int DisplayStatus();
    int Pull();
    int Build();

    const RepoConfig* GetRepo(const std::string_view& repo_name) const;

private:
    Config config;
    MultiControllerTasks tasks;

    bool ShouldExit() const;
    bool HasError() const;
    int RunTask(Display& display);
};
