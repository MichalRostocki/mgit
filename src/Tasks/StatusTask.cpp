#include "StatusTask.h"

#include "../Config.h"
#include "../GitLibLock.h"

std::unique_ptr<Task::TaskRunner> StatusTask::RegisterRepo(const RepoConfig& repository)
{
	return std::make_unique<StatusTaskRunner>(repository, &repositories_data.emplace_back());
}

void StatusTask::OnAllReposRegistered()
{
    for (const auto& data : repositories_data)
        max_name_space = std::max(data.repo_name.size(), max_name_space);
    max_name_space += 4;
}

size_t StatusTask::Display(std::ostream& output_stream)
{
    const auto data_copy = repositories_data;

    size_t max_size_of_branch_names = 11;
    for (const auto& data : data_copy)
    {
        if (!data.current_branch.empty())
        {
            size_t size_of_branch_name = data.current_branch.size() + 8;
            if (data.is_repo_detached)
                size_of_branch_name += 11;

            max_size_of_branch_names = std::max(max_size_of_branch_names, size_of_branch_name);
        }
    }
    max_size_of_branch_names += 4;

	for (const auto & data : data_copy)
	{
        output_stream << ' ' << data.repo_name;

        const auto spaces_needed = max_name_space - data.repo_name.size();
        for (size_t i = 0; i < spaces_needed; ++i)
            output_stream << ' ';

        if (data.is_repo_found)
        {
            size_t repo_branch_size = 8;
            output_stream << "branch: ";

            if (data.current_branch.empty())
            {
                output_stream << "...";
                repo_branch_size += 3;
            }
            else
            {
                output_stream << data.current_branch;
                repo_branch_size += data.current_branch.size();

                if (data.is_repo_detached)
                {
                    output_stream << " (DETACHED)";
                    repo_branch_size += 11;
                }
            }

            if (data.no_of_files_complete)
            {
                auto missing_spaces = max_size_of_branch_names - repo_branch_size;
                while (missing_spaces--)
                    output_stream << ' ';

                output_stream << "A: " << data.files_added << " M: " << data.files_modified << " D: " << data.files_deleted;
            }
        }
        else
        {
            output_stream << "- repository not found!";
        }

        output_stream << std::endl;
	}

	return data_copy.size();
}

bool StatusTask::ShouldExit()
{
	for (const auto & data : repositories_data)
        if (!data.is_complete)
            return false;

    return true;
}

bool StatusTask::IncludesHidden()
{
    return false;
}

StatusTask::StatusTaskRunner::StatusTaskRunner(const RepoConfig& repo, StatusRepoData* data) :
	TaskRunner(repo),
	data(data)
{
    data->repo_name = GetRepoName();
}

void StatusTask::StatusTaskRunner::Run()
{
    GitLibLock git;

    if(!git.OpenRepo(repo_config.path))
    {
        data->is_repo_found = false;
        GitError();
        return;
    }

    TASK_RUNNER_CHECK

    auto branch_data = git.GetBranchData();
    if(branch_data.failed)
    {
        GitError();
    	return;
    }

    data->is_repo_detached = branch_data.is_detached;
    data->current_branch = std::move(branch_data.branch_or_sha);

    TASK_RUNNER_CHECK

    const auto file_mode_stats = git.GetFileModificationStats();
    if(file_mode_stats.failed)
    {
        GitError();
    	return;
    }

    data->files_added = file_mode_stats.added;
    data->files_modified = file_mode_stats.modified;
    data->files_deleted = file_mode_stats.deleted;
    data->no_of_files_complete = true;

    data->is_complete = true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void StatusTask::StatusTaskRunner::GitError()
{
    data->is_complete = true;
    data->error_encountered = true;
}
