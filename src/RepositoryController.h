#pragma once
#include <filesystem>
#include <thread>

struct RepoConfig;

class RepositoryController
{
public:
    explicit RepositoryController(const RepoConfig& repo_config);
    ~RepositoryController();

    RepositoryController(const RepositoryController& other) = delete;
    RepositoryController(RepositoryController&& other) noexcept = delete;
    RepositoryController& operator=(const RepositoryController& other) = delete;
    RepositoryController& operator=(RepositoryController&& other) noexcept = delete;

    bool IsWorkerThreadFinished() const;
    bool IsValid() const;
    bool IsDataReady() const;
    bool HasFailed() const;

    std::string_view GetRepoName() const;
    std::string_view GetRepoCurrentBranch() const;
    bool IsRepoDetached() const;

private:
    const std::filesystem::path repository_path;
    const std::string repository_name;

    std::thread worker_thread;

    bool error_encountered;
    bool is_loaded;
    bool is_detached;

    std::string current_branch;

    void GitError();
    void WorkerProcedure();
};
