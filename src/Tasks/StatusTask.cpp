#include "StatusTask.h"

#include "../Config.h"
#include "../GitLibLock.h"

bool StatusTask::IsSuccessful()
{
	for (const auto & data : repositories_data)
	{
        if (!data.is_complete || !data.is_repo_found)
            return false;
	}

    return true;
}

std::unique_ptr<Task::TaskRunner> StatusTask::RegisterRepo(const RepoConfig& repository)
{
    if (repository.hidden)
        return nullptr;

	return std::make_unique<StatusTaskRunner>(repository, &repositories_data.emplace_back());
}

void StatusTask::OnAllReposRegistered()
{
    for (const auto& data : repositories_data)
        max_name_space = std::max(data.repo_config->repo_name.size(), max_name_space);
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
            size_of_branch_name += 2ull * data.repo_config->sub_repo_level;

            max_size_of_branch_names = std::max(max_size_of_branch_names, size_of_branch_name);
        }
    }
    max_size_of_branch_names += 4;

	for (const auto & data : data_copy)
	{
        const auto& repo_config = *data.repo_config;

        for (int i = 0; i < repo_config.sub_repo_level * 2; ++i)
            output_stream << ' ';

        output_stream << ' ' << repo_config.repo_name;

        const auto spaces_needed = max_name_space - (repo_config.repo_name.size() + 2ull * repo_config.sub_repo_level);
        for (size_t i = 0; i < spaces_needed; ++i)
            output_stream << ' ';

        if (data.is_repo_found)
        {
	        output_stream << "branch:";

            if (data.current_branch.empty())
            {
                output_stream << "...";
            }
            else
            {
                if (data.current_branch == data.repo_config->default_branch)
                    output_stream << ' ';
                else output_stream << '*';

                auto repo_branch_size = data.current_branch.size() + 8;
	            output_stream << data.current_branch;

                if (data.is_repo_detached)
                {
                    output_stream << " (DETACHED)";
                    repo_branch_size += 11;
                }

                if (data.no_of_files_complete)
                {
                    auto missing_spaces = max_size_of_branch_names - repo_branch_size;
                    while (missing_spaces--)
                    {
                        output_stream << ' ';
                    }

                	output_stream <<  "A: " << data.files_added
                				  << " M: " << data.files_modified
                				  << " D: " << data.files_deleted;
                }
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

StatusTask::StatusTaskRunner::StatusTaskRunner(const RepoConfig& repo, StatusRepoData* data) :
	TaskRunner(repo),
	data(data)
{
    data->repo_config = &repo;
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

    if(!git.GetBranchData(data->is_repo_detached, data->current_branch))
    {
        GitError();
    	return;
    }

    TASK_RUNNER_CHECK
        
    if(!git.GetFileModificationStats(should_stop, data->files_added, data->files_modified, data->files_deleted))
    {
        GitError();
    	return;
    }
    data->no_of_files_complete = true;

    data->is_complete = true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void StatusTask::StatusTaskRunner::GitError()
{
    data->is_complete = true;
    data->error_encountered = true;
}
