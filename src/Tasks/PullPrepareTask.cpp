#include "PullPrepareTask.h"

#include "Config.h"
#include "GitLibLock.h"
#include "RepoOrchestrator.h"

enum class PullPrepareStatus : uint8_t
{
	Preparing,
	OpeningRepo,
	CheckingHead,

	RemoteLookup,
	RemoteConnect,
	FetchingRemote,

	LocalLookup,
	LocalConnect,
	FetchingLocal,

	Comparing,
	Complete,

	// special
	RemoteStandard = RemoteLookup,
	RemoteLocal = LocalLookup
};

namespace
{
	PullPrepareStatus PpsConnect(const PullPrepareStatus remote_enum)
	{
		switch (remote_enum)
		{
		case PullPrepareStatus::RemoteStandard:
			return PullPrepareStatus::RemoteConnect;
		case PullPrepareStatus::RemoteLocal:
			return PullPrepareStatus::LocalConnect;
		default:
			throw std::exception("PpsConnect failure");
		}

	}

	PullPrepareStatus PpsFetching(const PullPrepareStatus remote_enum)
	{
		switch (remote_enum)
		{
		case PullPrepareStatus::RemoteStandard:
			return PullPrepareStatus::FetchingRemote;
		case PullPrepareStatus::RemoteLocal:
			return PullPrepareStatus::FetchingLocal;
		default:
			throw std::exception("PpsLookup failure");
		}
	}
}

namespace
{
	const char* ToString(const PullPrepareStatus e)
	{
		switch (e)
		{
		case PullPrepareStatus::Preparing: return "Preparing pull check";
		case PullPrepareStatus::OpeningRepo: return "Opening repository";
		case PullPrepareStatus::CheckingHead: return "Ensuring repo is not detached";

		case PullPrepareStatus::LocalLookup: return "Looking up local repository";
		case PullPrepareStatus::RemoteLookup: return "Looking up remote repository";

		case PullPrepareStatus::LocalConnect: return "Connecting to local repository";
		case PullPrepareStatus::FetchingLocal: return "Fetching local repository";

		case PullPrepareStatus::RemoteConnect: return "Connecting to remote repository";
		case PullPrepareStatus::FetchingRemote: return "Fetching remote repository";

		case PullPrepareStatus::Comparing: return "Comparing remote and local";
		case PullPrepareStatus::Complete: return "Complete";
		}

		return "Fetching...";
	}

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

	const auto& config = GetConfig();
	bool local_status = false;
	if (!config.local_repo.empty())
		local_status = FetchRemote(git, config.local_repo, PullPrepareStatus::RemoteLocal);
	TASK_RUNNER_CHECK;

	const bool remote_status = FetchRemote(git, DefaultRemote, PullPrepareStatus::RemoteStandard);
	TASK_RUNNER_CHECK;

	if (!local_status && !remote_status)
	{
		step_data.error = "No remote found";
		return false;
	}

	if (!remote_status && local_status)
		GetRepositoryInformation().has_only_local = true;

	TASK_RUNNER_CHECK;

	if (!Compare(git))
		return false;

	return true;
}

std::string_view PullPrepareTask::GetCommand()
{
	return ToString(status);
}

// ReSharper disable once CppMemberFunctionMayBeConst
int PullPrepareTask::FetchRemoteCommand(const char* str)
{
	if (should_stop)
		return -1;

	step_data.output << "remote: " << str << '\n';
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
		<< bytes << "bytes" << '\n';
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
	step_data.output << "Repository " << config.repo_name << " found" << '\n';

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

	step_data.output << "Current branch is " << info.current_branch << '\n';

	return true;
}

bool PullPrepareTask::FetchRemote(GitLibLock& git, const std::string_view& remote, const PullPrepareStatus remote_enum)
{
	std::function<int(const char*)> remote_text_func = [this](const char* str)
	{
		return FetchRemoteCommand(str);
	};

	std::function<int(unsigned, unsigned, size_t)> transfer_func = [this](const unsigned processed, const unsigned total, const size_t bytes)
	{
		return FetchTransferCommand(processed, total, bytes);
	};

	if (!remote.empty())
	{
		status = remote_enum;

		if (git.LookupRemote(remote))
		{
			step_data.output << "Remote " << remote << " found\n";

			TASK_RUNNER_CHECK;
			status = PpsConnect(remote_enum);

			if (git.ConnectToRemote(remote))
			{
				step_data.output << "Connected to repository " << remote << '\n';

				TASK_RUNNER_CHECK;
				status = PpsFetching(remote_enum);

				if (git.Fetch(remote_text_func, transfer_func, remote))
				{
					step_data.output << "Fetched data from repository " << remote << '\n';
					return true;
				}
			}
			else step_data.output << "Failed to connect to remote " << remote << '\n';
		}
		else step_data.output << "Failed to lookup remote " << remote << '\n';
	}

	return false;
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
		return true; // repo is detached, we won't modify it anyway

	TASK_RUNNER_CHECK;

	if (git.HasIncoming())
		info.has_incoming = true;
	else return true; // nothing is incoming, we won't modify it anyway

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
