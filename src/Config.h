#pragma once
#include "json.hpp"

struct ErrorHandling
{
    bool retry = false;
    std::vector<std::string> before_retry;
};

struct BuildConfig
{
    std::string working_dir;
    std::vector<std::string> require;
    std::vector<std::string> require_pull;
    std::vector<std::string> steps;
    std::vector<std::string> env;
    ErrorHandling on_error;
};

struct RepoConfig
{
    std::string path;
    std::string default_branch;
    bool hidden = false;

    std::vector<RepoConfig> sub_repos;

    BuildConfig build;

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
void from_json(const nlohmann::json& j, ErrorHandling& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, BuildConfig& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p);

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p);
