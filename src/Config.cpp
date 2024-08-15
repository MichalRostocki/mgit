#include "Config.h"

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p)
{
    j.at("path").get_to(p.Path);
    j.at("default_branch").get_to(p.DefaultBranch);
    if (j.contains("priority"))
        j.at("priority").get_to(p.Priority);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p)
{
    j.at("repositories").get_to(p.Repositories);
}
