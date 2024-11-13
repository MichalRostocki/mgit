#include <iostream>

#include "MultiController.h"

int ShowUsage()
{
    std::cout << "Usage: 'mgit <command>'" << std::endl
        << "where:" << std::endl
        << "\tstatus - displays status for all repositories" << std::endl;
    return 0;
}

int DisplayStatus()
{
    MultiController ctr;
    std::ostringstream error_stream;

    if(!ctr.LoadConfig(error_stream))
    {
        std::cout << error_stream.rdbuf();
        return 1;
    }

    return ctr.DisplayStatus();
}

int TryRunGitCli(const RepoConfig* repo_config, const std::vector<std::string>& args)
{
    std::stringstream buffer;
	buffer << "git ";
    buffer << "-C " << repo_config->path << ' ';

    for (const auto & arg : args)
    {
        buffer << arg;
        buffer << ' ';
    }

    const std::string command{ buffer.str() };
    return std::system(command.c_str());
}

int TryActivateRepo(const std::string_view& repo_name, const std::vector<std::string>& args)
{
    MultiController ctr;
    std::ostringstream error_stream;

    if (!ctr.LoadConfig(error_stream))
    {
        std::cout << error_stream.rdbuf();
        return 1;
    }

    if(const auto* repo_config = ctr.GetRepo(repo_name))
        return TryRunGitCli(repo_config, args);

    std::cout << "Command not specified" << std::endl;
    return 1;
}

int BuildRepos()
{
    MultiController ctr;
    std::ostringstream error_stream;

    if (!ctr.LoadConfig(error_stream))
    {
        std::cout << error_stream.rdbuf();
        return 1;
    }

    return ctr.Build();
}

int PullRepos()
{
    MultiController ctr;
    std::ostringstream error_stream;

    if (!ctr.LoadConfig(error_stream))
    {
        std::cout << error_stream.rdbuf();
        return 1;
    }

    return ctr.Pull();
}

int HandleCommand(const std::string_view& command, const std::vector<std::string>& args)
{
    if(command == "help")
        return ShowUsage();
    if (command == "status")
        return DisplayStatus();
    if (command == "build")
        return BuildRepos();
    if (command == "pull")
        return PullRepos();

    return TryActivateRepo(command, args);
}

int main(const int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cout << "No command found" << std::endl;
        ShowUsage();
        return 1;
    }

    std::vector<std::string> arguments;

    for (int i = 2; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    return HandleCommand(argv[1], arguments);
}
