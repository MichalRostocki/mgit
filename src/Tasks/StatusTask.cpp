#include "StatusTask.h"

#include <Config.h>
#include <GitLibLock.h>

#include "RepoOrchestrator.h"

StatusTask::StatusTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	Task(repo_orchestrator, step)
{
}

bool StatusTask::Run()
{
    GitLibLock git;
    auto& info = parent->GetRepositoryInfo();

    if (!git.OpenRepo(parent->GetConfig().path))
    {
        info.is_repo_found = false;
        step_data.error = "Couldn't find repository";
        return false;
    }

    TASK_RUNNER_CHECK;

    bool is_repo_detached = false;

	if (!git.GetBranchData(is_repo_detached, info.current_branch))
    {
        step_data.error = "Couldn't get branch data";
        return false;
    }
    info.is_repo_detached = is_repo_detached;

    TASK_RUNNER_CHECK;

    if (!git.GetFileModificationStats(should_stop, info.files_added, info.files_modified, info.files_deleted))
    {
        step_data.error = "Couldn't read modification stats";
        return false;
    }
    info.no_of_files_complete = true;

    return true;
}

std::string_view StatusTask::GetCommand()
{
    return "git status";
}
