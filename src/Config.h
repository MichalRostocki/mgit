#pragma once
#include "json.hpp"

struct RepoConfig
{
    std::string path;
    std::string default_branch;
    bool hidden = false;
};

struct Config
{
    std::vector<RepoConfig> repositories;

    bool IsValid() const
    {
        return !repositories.empty();
    }
};

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p);
