#include "StatusDisplay.h"

#include "Data/StatusData.h"

StatusDisplay::StatusDisplay(const std::list<StatusData>& data) : data_collection(data), max_name_space(0)
{
    for (const auto& status_data : data_collection)
        max_name_space = std::max(status_data.repo_name.size(), max_name_space);
    max_name_space += 4;
}

size_t StatusDisplay::Print(std::ostream& output, bool will_exit)
{
    const auto data_copy = data_collection;

    size_t max_size_of_branch_names = 11;
    for (const auto& data : data_copy)
    {
        if (!data.current_branch.empty())
        {
            size_t size_of_branch_name = data.current_branch.size() + 8;
            if (data.is_repo_detached)
                size_of_branch_name += 11;
            size_of_branch_name += 2ull * data.sub_repo_level;

            max_size_of_branch_names = std::max(max_size_of_branch_names, size_of_branch_name);
        }
    }
    max_size_of_branch_names += 4;

    for (const auto& data : data_copy)
    {
        for (size_t i = 0; i < data.sub_repo_level * 2; ++i)
            output << ' ';

        output << ' ' << data.repo_name;

        const auto spaces_needed = max_name_space - (data.repo_name.size() + 2ull * data.sub_repo_level);
        for (size_t i = 0; i < spaces_needed; ++i)
            output << ' ';

        if (data.is_repo_found)
        {
            output << "branch:";

            if (data.current_branch.empty())
            {
                output << "...";
            }
            else
            {
                if (data.current_branch == data.default_branch)
                    output << ' ';
                else output << '*';

                auto repo_branch_size = data.current_branch.size() + 8;
                output << data.current_branch;

                if (data.is_repo_detached)
                {
                    output << " (DETACHED)";
                    repo_branch_size += 11;
                }

                if (data.no_of_files_complete)
                {
                    auto missing_spaces = max_size_of_branch_names - repo_branch_size;
                    while (missing_spaces--)
                    {
                        output << ' ';
                    }

                    output << "A: " << data.files_added
                        << " M: " << data.files_modified
                        << " D: " << data.files_deleted;
                }
            }
        }
        else
        {
            output << "- repository not found!";
        }

        output << std::endl;
    }

    return data_copy.size();
}
