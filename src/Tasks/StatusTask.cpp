#include "StatusTask.h"

#include <Config.h>
#include <Data/StatusData.h>
#include <GitLibLock.h>

StatusTask::StatusTask(const RepoConfig& config, StatusData& data):
	Task(config),
	data(data)
{
}

void StatusTask::Run()
{
    GitLibLock git;

    if (!git.OpenRepo(repo_config.path))
    {
        data.is_repo_found = false;
        error_encountered = true;
        return;
    }

    TASK_RUNNER_CHECK;

	if (!git.GetBranchData(data.is_repo_detached, data.current_branch))
    {
	    error_encountered = true;
        return;
    }

    TASK_RUNNER_CHECK;

    if (!git.GetFileModificationStats(should_stop, data.files_added, data.files_modified, data.files_deleted))
    {
        error_encountered = true;
        return;
    }
    data.no_of_files_complete = true;
}
