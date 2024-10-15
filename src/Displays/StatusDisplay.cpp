#include "StatusDisplay.h"

#include "Config.h"
#include "RepoOrchestrator.h"
#include "OrderedMap.h"

StatusDisplay::StatusDisplay(const MultiControllerTasks& data) : data_collection(data), max_name_space(0)
{
    for (const auto& status_data : data_collection)
        max_name_space = std::max(status_data.second->GetConfig().repo_name.size(), max_name_space);
    max_name_space += 4;
}

size_t StatusDisplay::Print(std::ostream& output, bool will_exit)
{
    size_t max_size_of_branch_names = 11;
    for (const auto& data : data_collection)
    {
        const auto& repo_info = data.second->GetRepositoryInfo();
        if (!repo_info.current_branch.empty())
        {
            size_t size_of_branch_name = repo_info.current_branch.size() + 8;
            if (repo_info.is_repo_detached)
                size_of_branch_name += 11;
            size_of_branch_name += 2ull * repo_info.sub_repo_level;

            max_size_of_branch_names = std::max(max_size_of_branch_names, size_of_branch_name);
        }
    }
    max_size_of_branch_names += 4;

    for (const auto& data : data_collection)
    {
        const auto& repo_info = data.second->GetRepositoryInfo();
        const auto& repo_config = data.second->GetConfig();

        for (size_t i = 0; i < repo_info.sub_repo_level * 2; ++i)
            output << ' ';

        output << ' ' << repo_config.repo_name;

        const auto spaces_needed = max_name_space - (repo_config.repo_name.size() + 2ull * repo_info.sub_repo_level);
        for (size_t i = 0; i < spaces_needed; ++i)
            output << ' ';

        if (repo_info.is_repo_found)
        {
            output << "branch:";

            if (repo_info.current_branch.empty())
            {
                output << "...";
            }
            else
            {
                if (repo_info.current_branch == repo_config.default_branch)
                    output << ' ';
                else output << '*';

                auto repo_branch_size = repo_info.current_branch.size() + 8;
                output << repo_info.current_branch;

                if (repo_info.is_repo_detached)
                {
                    output << " (DETACHED)";
                    repo_branch_size += 11;
                }

                if (repo_info.no_of_files_complete)
                {
                    auto missing_spaces = max_size_of_branch_names - repo_branch_size;
                    while (missing_spaces--)
                    {
                        output << ' ';
                    }

                    output << "A: " << repo_info.files_added
                        << " M: " << repo_info.files_modified
                        << " D: " << repo_info.files_deleted;
                }
            }
        }
        else
        {
            output << "- repository not found!";
        }

        output << std::endl;
    }

    return data_collection.size();
}
