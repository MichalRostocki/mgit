#include "PullPrepareTask.h"

#include "Config.h"
#include "GitLibLock.h"
#include "RepoOrchestrator.h"

enum class PullPrepareStatus
{
	Preparing,
	OpeningRepo,
	CheckingHead,
	RemoteLookup,
	RemoteConnect,
	Fetching,
	Comparing,
	Complete
};

namespace
{
	const std::map<PullPrepareStatus, std::string> StatusStrings{
		{PullPrepareStatus::Preparing, "Preparing pull check"},
		{PullPrepareStatus::OpeningRepo, "Opening repository"},
		{PullPrepareStatus::CheckingHead, "Ensuring repo is not detached"},
		{PullPrepareStatus::RemoteLookup, "Looking up remote repository"},
		{PullPrepareStatus::RemoteConnect, "Connecting to remote repository"},
		{PullPrepareStatus::Fetching, "Fetching content"},
		{PullPrepareStatus::Comparing, "Comparing remote and local"},
		{PullPrepareStatus::Complete, "Complete"},
	};

	auto DefaultRemote = "origin";
}

PullPrepareTask::PullPrepareTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	Task(repo_orchestrator, step),
	status(PullPrepareStatus::Preparing)
{
}

bool PullPrepareTask::Run()
{
	GitLibLock git;

	if (!Prepare(git))
		return false;

	if (!Fetch(git))
		return false;

	if (!Compare(git))
		return false;

	return true;
}

std::string_view PullPrepareTask::GetCommand()
{
	const auto it = StatusStrings.find(status);
	if (it != StatusStrings.end())
		return it->second;
	return "Fetching...";
}

// ReSharper disable once CppMemberFunctionMayBeConst
int PullPrepareTask::FetchRemoteCommand(const char* str)
{
	if (should_stop)
		return -1;

	step_data.output << "remote: " << str << std::endl;
	return 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
int PullPrepareTask::FetchTransferCommand(const unsigned processed, const unsigned total, const size_t bytes)
{
	if (should_stop)
		return -1;

	step_data.output << "Processed: "
		<< processed << " / "<< total
		<< " (" << 100.f * (static_cast<float>(processed) / static_cast<float>(total)) << "): "
		<< bytes << "bytes" << std::endl;
	return 0;
}

bool PullPrepareTask::Prepare(GitLibLock& git)
{
	auto& info = GetRepositoryInformation();
	const auto& config = GetConfig();

	status = PullPrepareStatus::OpeningRepo;
	if (!git.OpenRepo(config.path))
	{
		info.is_repo_found = false;
		step_data.error = "Couldn't find repository";
		return false;
	}
	step_data.output << "Repository " << config.repo_name << " found" << std::endl;

	TASK_RUNNER_CHECK;

	status = PullPrepareStatus::CheckingHead;
	bool is_repo_detached = false;
	if (!git.GetBranchData(is_repo_detached, info.current_branch))
	{
		step_data.error = "Couldn't get branch data";
		return false;
	}

	if (is_repo_detached)
	{
		step_data.error = "This repository is detached, cannot pull from it";
		return false;
	}

	step_data.output << "Current branch is " << info.current_branch << std::endl;

	TASK_RUNNER_CHECK;

	status = PullPrepareStatus::RemoteLookup;
	if (!git.LookupRemote(DefaultRemote))
	{
		step_data.error = "Failed to lookup remote origin";
		return false;
	}

	step_data.output << "Remote origin found" << std::endl;

	TASK_RUNNER_CHECK;

	return true;
}

bool PullPrepareTask::Fetch(GitLibLock& git)
{
	status = PullPrepareStatus::RemoteConnect;

	std::function<int(const char*)> remote_text_func = [this](const char* str)
	{
		return FetchRemoteCommand(str);
	};

	std::function<int(unsigned, unsigned, size_t)> transfer_func = [this](const unsigned processed, const unsigned total, const size_t bytes)
	{
		return FetchTransferCommand(processed, total, bytes);
	};

	if (!git.ConnectToRemote(remote_text_func, transfer_func, DefaultRemote))
	{
		step_data.error = "Failed to connect to repository origin";
		return false;
	}

	step_data.output << "Connected to repository origin" << std::endl;
	status = PullPrepareStatus::Fetching;

	if(!git.Fetch(remote_text_func, transfer_func, DefaultRemote))
	{
		step_data.error = "Failed to fetch remote repository";
		return false;
	}

	TASK_RUNNER_CHECK;

	step_data.output << "Fetched data from remote repository" << std::endl;
	return true;
}

bool PullPrepareTask::Compare(GitLibLock& git)
{
	status = PullPrepareStatus::Comparing;

	auto& info = GetRepositoryInformation();
	auto is_detached = false;

	if (!git.GetBranchData(is_detached, info.current_branch))
	{
		step_data.error = "Couldn't get branch data";
		return false;
	}

	info.is_repo_detached = is_detached;
	if (is_detached)
		return true; // repo is detached, we won't modify it anyways

	TASK_RUNNER_CHECK;

	if (git.HasIncoming())
		info.has_incoming = true;
	else return true; // nothing is incoming, we won't modify it anyways

	TASK_RUNNER_CHECK;

	if (!git.GetFileModificationStats(should_stop, info.files_added, info.files_modified, info.files_deleted))
	{
		step_data.error = "Couldn't read modification stats";
		return false;
	}

	return true;
}

SubmodulePullPrepareTask::SubmodulePullPrepareTask(RepoOrchestrator* repo_orchestrator, StepData& step,
	const RepoConfig& submodule_config, RepositoryInformation& submodule_information) :
	PullPrepareTask(repo_orchestrator, step),
	submodule_config(submodule_config),
	submodule_information(submodule_information)
{
}

const RepoConfig& SubmodulePullPrepareTask::GetConfig() const
{
	return submodule_config;
}

RepositoryInformation& SubmodulePullPrepareTask::GetRepositoryInformation() const
{
	return submodule_information;
}
