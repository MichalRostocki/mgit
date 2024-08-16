#include "Config.h"

std::string RepoConfig::GetRepoName() const
{
    return std::filesystem::path{ path }.filename().string();
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p)
{
    j.at("path").get_to(p.path);
    j.at("default_branch").get_to(p.default_branch);
    if (j.contains("hidden"))
        j.at("hidden").get_to(p.hidden);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p)
{
    j.at("repositories").get_to(p.repositories);
}
