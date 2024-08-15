#pragma once
#include "json.hpp"

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
void from_json(const nlohmann::json& j, RepoConfig& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p);
