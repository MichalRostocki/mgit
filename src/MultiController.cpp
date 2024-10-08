#include "MultiController.h"

#include <fstream>

#include "Config.h"
#include "Tasks/BuildTask.h"
#include "Tasks/StatusTask.h"

constexpr auto ConfigFilePathAppendix = R"(\.config\mgit\repos.json)";

namespace OutputControl
{
    void ClearLine(std::ostream& output_stream) {
        output_stream << "\33[2K\r";
    }

    void MoveCursorUp(std::ostream& output_stream, const size_t lines) {
        output_stream << "\33[" << lines << "A";
    }

    void MoveCursorDown(std::ostream& output_stream, const size_t lines) {
        output_stream << "\33[" << lines << "B";
    }
}

bool MultiController::LoadConfig(std::ostream& error_stream)
{
    // ReSharper disable once StringLiteralTypo
    // ReSharper disable once CppDeprecatedEntity
    const auto user_profile = getenv("USERPROFILE");
    const auto config_file_path = std::string{ user_profile } + ConfigFilePathAppendix;

    std::ifstream f{ config_file_path };
    if (!f.is_open())
    {
        error_stream << "Could not read config or it's empty. "
            << "Localization: " << config_file_path;
        return false;
    }

    const nlohmann::json data = nlohmann::json::parse(f);
    f.close();

    config = data.get<Config>();

    return config.Validate();
}

int MultiController::DisplayStatus(std::ostream& output_stream) const
{
    StatusTask task;
    return RunTask(&task, output_stream);
}

int MultiController::Build(std::ostream& output_stream) const
{
    BuildTask task;
    return RunTask(&task, output_stream);
}

const RepoConfig* MultiController::GetRepo(const std::string_view& repo_name) const
{
	for (const auto & repo_config : config.repositories)
	{
        if(repo_config.repo_name == repo_name)
        {
            return &repo_config;
        }
	}

	return nullptr;
}

int MultiController::RunTask(Task* task, std::ostream& output_stream) const
{
    if (!task)
        return 1;

    task->Register(config);
    task->Process(output_stream);

    return task->IsSuccessful() ? 0 : 1;
}
