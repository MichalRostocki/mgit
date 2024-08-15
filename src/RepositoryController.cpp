#include "RepositoryController.h"

#include <git2/errors.h>
#include <git2/refs.h>
#include <git2/repository.h>

#include "Config.h"
#include "GitLibLock.h"

namespace
{
    bool GitDirectoryExists(const std::filesystem::path& path)
    {
        const auto git_repo_path = std::filesystem::path{ path }.append(".git");
        return exists(git_repo_path);
    }
}

RepositoryController::RepositoryController(const RepoConfig& repo_config) :
    repository_path(repo_config.Path),
    repository_name(repository_path.filename().string()),
    error_encountered(false),
    is_loaded(false),
	is_detached(false)
{
    if(IsValid())
    {
        worker_thread = std::thread([this] {this->WorkerProcedure(); });
    }
    else
    {
        error_encountered = true;
        is_loaded = true;
    }
}

RepositoryController::~RepositoryController()
{
	worker_thread.join();
}

bool RepositoryController::IsWorkerThreadFinished() const
{
    return worker_thread.joinable();
}

bool RepositoryController::IsValid() const
{
    return !repository_name.empty()
        && exists(repository_path)
        && GitDirectoryExists(repository_path);
}

bool RepositoryController::IsDataReady() const
{
    return is_loaded;
}

bool RepositoryController::HasFailed() const
{
    return error_encountered;
}

std::string_view RepositoryController::GetRepoName() const
{
    return repository_name;
}

std::string_view RepositoryController::GetRepoCurrentBranch() const
{
    return current_branch;
}

bool RepositoryController::IsRepoDetached() const
{
    return is_detached;
}

void RepositoryController::GitError()
{
    error_encountered = true;
    is_loaded = true;
}

void RepositoryController::WorkerProcedure()
{
    GitLibLock git;

    if(git_repository_open_ext(&git.repository, repository_path.string().c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, "") != GIT_ERROR_NONE)
    {
        GitError();
        return;
    }

    if(git_repository_head(&git.head, git.repository) != GIT_ERROR_NONE)
    {
        GitError();
        return;
    }

    is_detached = !git_reference_is_branch(git.head);
    if(is_detached)
    {
        char sha[GIT_OID_SHA1_HEXSIZE + 1];
        const git_oid* oid = git_reference_target(git.head);
        git_oid_tostr(sha, sizeof(sha), oid);
        current_branch = sha;
    }
    else
    {
        current_branch = git_reference_shorthand(git.head);
    }

    is_loaded = true;
}
