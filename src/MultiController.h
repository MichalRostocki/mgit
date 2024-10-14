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
