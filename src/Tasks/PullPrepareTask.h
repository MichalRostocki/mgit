#pragma once
#include <string>

#include "Task.h"

struct RepoConfig;
struct RepositoryInformation;
class GitLibLock;
enum class PullPrepareStatus;

class PullPrepareTask : public Task
{
public:
	explicit PullPrepareTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	bool Run() override;
	std::string_view GetCommand() override;

	// internal
	int FetchRemoteCommand(const char* str);
	int FetchTransferCommand(unsigned processed, unsigned total, size_t bytes);

private:
	PullPrepareStatus status;

	bool Prepare(GitLibLock& git);
	bool Fetch(GitLibLock& git); 
	bool Compare(GitLibLock& git);
};

class SubmodulePullPrepareTask final : public PullPrepareTask
{
public:
	explicit SubmodulePullPrepareTask(RepoOrchestrator* repo_orchestrator, StepData& step, const RepoConfig& submodule_config, RepositoryInformation& submodule_information);

private:
	const RepoConfig& GetConfig() const override;
	RepositoryInformation& GetRepositoryInformation() const override;

	const RepoConfig& submodule_config;
	RepositoryInformation& submodule_information;
};
