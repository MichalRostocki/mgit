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

	for (const auto & data : data_copy)
	{
        output_stream << ' ' << data.repo_name;

        const auto spaces_needed = max_name_space - data.repo_name.size();
        for (size_t i = 0; i < spaces_needed; ++i)
            output_stream << ' ';

        if (data.is_repo_found)
        {
            output_stream << "branch: ";

            if (data.current_branch.empty())
            {
                output_stream << "...";
            }
            else
            {
                output_stream << data.current_branch;
                if (data.is_repo_detached)
                    output_stream << " (DETACHED)";
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
    data->repo_name = GetRepoName();
}

void StatusTask::StatusTaskRunner::Run()
{
    GitLibLock git;

    if(!git.OpenRepo(repo_config.Path))
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

    data->is_complete = true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void StatusTask::StatusTaskRunner::GitError()
{
    data->is_complete = true;
    data->error_encountered = true;
}
