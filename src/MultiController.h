#pragma once
#include "Config.h"

class Task;

class MultiController
{
public:
    bool LoadConfig(std::ostream& error_stream);

    void DisplayStatus(std::ostream& output_stream) const;

    const RepoConfig* GetRepo(const std::string_view& repo_name) const;

private:
    Config config;

    void RunTask(Task* task, std::ostream& output_stream) const;
};

