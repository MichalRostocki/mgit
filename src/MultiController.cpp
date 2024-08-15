#include "MultiController.h"

#include <fstream>

#include "Config.h"

constexpr auto ConfigFilePathAppendix = R"(.config\mgit\repos.json)";

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

    const auto config = data.get<Config>();
    for (const auto& repo_config : config.Repositories)
    {
        controllers.emplace_back(repo_config);
        if(controllers.back().IsValid())
        {
            error_stream << "Repository " << repo_config.Path
                << " was not found";
        }
    }

    return true;
}

void MultiController::DisplayStatus(std::ostream& output_stream) const
{
    output_stream << "Found " << controllers.size() << " repositories" << std::endl;

    size_t name_space = 0;

    for (const auto & controller : controllers)
        name_space = std::max(controller.GetRepoName().size(), name_space);
    name_space += 4;

    bool is_finished;

    do
    {
        is_finished = true;
        OutputControl::MoveCursorUp(output_stream, controllers.size());

        for (const auto& controller : controllers)
        {
            OutputControl::ClearLine(output_stream);

            const auto name = controller.GetRepoName();
            output_stream << ' ' << name;

            const auto spaces_needed = name_space - name.size();
            for (size_t i = 0; i < spaces_needed; ++i)
                output_stream << ' ';

            if (controller.IsValid())
            {
                if (!controller.IsDataReady())
                    is_finished = false;

                output_stream << "branch: ";

                if (const auto branch = controller.GetRepoCurrentBranch();
                    branch.empty())
                {
                    output_stream << "...";
                }
                else
                {
                    output_stream << branch;
                    if (controller.IsRepoDetached())
                        output_stream << " (DETACHED)";
                }
            }
            else
            {
                output_stream << "- repository not found!";
            }

            output_stream << std::endl;
        }

        if (!is_finished)
            std::this_thread::sleep_for(std::chrono::milliseconds{100});

    } while (!is_finished);
}

MultiController::~MultiController()
{
    constexpr auto max_step = 10;
    auto step = 0;
    bool all_joined;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 2 * step });
        all_joined = true;

        for (const auto & controller : controllers)
        {
            if (!controller.IsWorkerThreadFinished())
                all_joined = false;
        }

        step++;

    } while (!all_joined && max_step != step);
}
