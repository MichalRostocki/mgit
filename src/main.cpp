#include <fstream>
#include <iostream>

#include "json.hpp"
using Json = nlohmann::json;

constexpr auto CONFIG_FILE_PATH_APPENDIX = R"(\.config\mgit\repos.json)";

struct RepoConfig
{
    std::string Path;
    std::string DefaultBranch;
    bool Priority = false;
};

struct Config
{
    std::vector<RepoConfig> Repositories;

    bool IsValid() const
    {
        return !Repositories.empty();
    }
};

// ReSharper disable once CppInconsistentNaming
void from_json(const Json& j, RepoConfig& p) {
    j.at("path").get_to(p.Path);
    j.at("default_branch").get_to(p.DefaultBranch);
    if(j.contains("priority"))
        j.at("priority").get_to(p.Priority);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const Json& j, Config& p) {
    j.at("repositories").get_to(p.Repositories);
}

Config GetConfig()
{
    // ReSharper disable once StringLiteralTypo
    // ReSharper disable once CppDeprecatedEntity
    const auto user_profile = getenv("USERPROFILE");
    const auto config_file_path = std::string{ user_profile } + CONFIG_FILE_PATH_APPENDIX;

    std::ifstream f{ config_file_path };
    if(!f.is_open())
    {
        std::cout << "Could not read config or it's empty" << std::endl
            << "Localization: " << config_file_path << std::endl;
        return {};
    }

    Json data = Json::parse(f);
    return data.get<Config>();
}

int ShowUsage()
{
    std::cout << "Usage: 'mgit <command>'" << std::endl
        << "where:" << std::endl
        << "\tstatus - displays status for all repositories" << std::endl;
    return 0;
}

int DisplayStatus()
{
    const auto config = GetConfig();
    if(!config.IsValid())
    {
        std::cout << "Config is empty" << std::endl;
        return 1;
    }

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
