#pragma once
#include "json.hpp"

struct RepoConfig
{
    std::string path;
    std::string default_branch;
    bool hidden = false;

    std::vector<RepoConfig> sub_repos;

    // calculated
    std::string repo_name;
    uint8_t sub_repo_level = 0;

    bool Validate();
};

struct Config
{
    std::vector<RepoConfig> repositories;

    bool Validate();
};

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p);
