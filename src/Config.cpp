#include "Config.h"

namespace
{
	bool GitExists(const std::filesystem::path& path)
	{
        auto git_path{ path };
        git_path.append(".git");
        return exists(git_path);
	}
}

bool RepoConfig::Validate()
{
	const std::filesystem::path filepath{ path };
    if (!exists(filepath) || !GitExists(filepath))
        return false;

    repo_name = filepath.filename().string();

    for (auto& sub_repo : sub_repos)
    {
        if (!sub_repo.Validate())
            return false;

        sub_repo.sub_repo_level = sub_repo_level + 1;
    }

    return true;
}

bool Config::Validate()
{
    bool is_valid = true;

	for (auto& repository : repositories)
        is_valid &= repository.Validate();

    return true;
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, ErrorHandling& p)
{
    if (j.contains("retry"))
        j.at("retry").get_to(p.retry);
    if (j.contains("before_retry"))
        j.at("before_retry").get_to(p.before_retry);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, BuildConfig& p)
{
    if (j.contains("require"))
        j.at("require").get_to(p.require);
    if (j.contains("require_pull"))
        j.at("require_pull").get_to(p.require_pull);
    if (j.contains("steps"))
        j.at("steps").get_to(p.steps);
    if (j.contains("working_dir"))
        j.at("working_dir").get_to(p.working_dir);
    if (j.contains("env"))
        j.at("env").get_to(p.env);
    if (j.contains("on_error"))
        j.at("on_error").get_to(p.on_error);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, RepoConfig& p)
{
    j.at("path").get_to(p.path);
    j.at("default_branch").get_to(p.default_branch);
    if (j.contains("hidden"))
        j.at("hidden").get_to(p.hidden);
    if (j.contains("sub_repos"))
        j.at("sub_repos").get_to(p.sub_repos);
    if (j.contains("build"))
        j.at("build").get_to(p.build);
}

// ReSharper disable once CppInconsistentNaming
void from_json(const nlohmann::json& j, Config& p)
{
    j.at("repositories").get_to(p.repositories);
}
