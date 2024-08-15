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

    ctr.DisplayStatus(std::cout);
    return 0;
}

int HandleCommand(const std::string_view& command)
{
    if(command == "help")
        return ShowUsage();
    if (command == "status")
        return DisplayStatus();

    return 1;
}

int main(const int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cout << "No command found" << std::endl;
        ShowUsage();
        return 1;
    }

    return HandleCommand(argv[1]);
}
