#include "GitLibLock.h"

#include <filesystem>
#include <git2.h>

GitLibLock::GitLibLock()
{
	git_libgit2_init();
}

GitLibLock::~GitLibLock()
{
	if (index)
		git_index_free(index);

	if (remote)
		git_remote_free(remote);

	if (head)
		git_reference_free(head);

	if (status_list)
		git_status_list_free(status_list);

	if (repository)
		git_repository_free(repository);

	git_libgit2_shutdown();
}

bool GitLibLock::OpenRepo(const std::string_view& path)
{
	if (repository)
		return false;

	return git_repository_open_ext(&repository, path.data(), GIT_REPOSITORY_OPEN_NO_SEARCH, "") == GIT_ERROR_NONE;
}

bool GitLibLock::LookupRemote()
{
	if (!repository)
		return false;

	if (!remote)
		if (git_remote_lookup(&remote, repository, "origin") != GIT_ERROR_NONE)
			return false;

	return true;
}

namespace ConnectToRemoteUtils
{
	struct ConnectData
	{
		std::function<int(const char*)>& remote_text_callback;
		std::function<int(unsigned, unsigned, size_t)>& transfer_callback;
	};

	int RemoteTextCallback(const char* str, int, void* user_data)
	{
		const auto fetch_data = static_cast<ConnectData*>(user_data);
		return fetch_data->remote_text_callback(str);
	}

	int TransferProgressCallback(const git_indexer_progress* progress, void* user_data)
	{
		const auto fetch_data = static_cast<ConnectData*>(user_data);
		const unsigned processed = progress->indexed_deltas + progress->indexed_objects + progress->received_objects;
		const unsigned total = progress->total_deltas + progress->total_objects * 2;
		return fetch_data->transfer_callback(processed, total, progress->received_bytes);
	}
}

bool GitLibLock::ConnectToRemote()
{
	std::function<int(const char*)> f1 = [](const char*) {return 0; };
	std::function<int(unsigned, unsigned, size_t)> f2 = [](unsigned, unsigned, size_t) {return 0; };

	return ConnectToRemote(f1, f2);
}

bool GitLibLock::ConnectToRemote(std::function<int(const char*)>& remote_text_callback,
                                 std::function<int(unsigned, unsigned, size_t)>& progress_callback)
{
	if (!remote)
		if (!LookupRemote())
			return false;

	using namespace ConnectToRemoteUtils;
	ConnectData connect_data{ remote_text_callback, progress_callback };

	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	callbacks.transfer_progress = &TransferProgressCallback;
	callbacks.sideband_progress = &RemoteTextCallback;
	callbacks.payload = &connect_data;

	const auto error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr);
	return error == GIT_OK;
}

bool GitLibLock::Fetch(std::function<int(const char*)>& remote_text_callback,
	std::function<int(unsigned, unsigned, size_t)>& progress_callback)
{
	if (!repository)
		return false;

	if (!remote)
		if (!LookupRemote())
			return false;

	using namespace ConnectToRemoteUtils;
	ConnectData connect_data{ remote_text_callback, progress_callback };

	git_fetch_options fetch_options = GIT_FETCH_OPTIONS_INIT;
	fetch_options.callbacks.transfer_progress = &TransferProgressCallback;
	fetch_options.callbacks.sideband_progress = &RemoteTextCallback;
	fetch_options.callbacks.payload = &connect_data;

	const auto error = git_remote_fetch(remote, nullptr, &fetch_options, nullptr);
	return error == GIT_OK;
}

bool GitLibLock::FullCheckoutToIndex()
{
	if (!index)
		if (!GetCurrentIndex())
			return false;

	git_checkout_options options = GIT_CHECKOUT_OPTIONS_INIT;
	options.checkout_strategy = GIT_CHECKOUT_FORCE;

	return git_checkout_index(repository, index, &options) == GIT_OK;
}

bool GitLibLock::GetBranchData(bool& is_detached, std::string& branch_or_sha)
{
	if (!head)
		if (!GetHead())
			return false;

	is_detached = !git_reference_is_branch(head);

	if (is_detached)
	{
		char sha[GIT_OID_SHA1_HEXSIZE + 1];
		const git_oid* oid = git_reference_target(head);
		git_oid_tostr(sha, sizeof(sha), oid);
		branch_or_sha = sha;
	}
	else
	{
		branch_or_sha = git_reference_shorthand(head);
	}
	
	return true;
}

bool GitLibLock::GetFileModificationStats(const bool& interrupt, size_t& added, size_t& modified, size_t deleted)
{
	constexpr static git_status_options Opts = GIT_STATUS_OPTIONS_INIT;

	if (!repository)
		return false;

	if (git_status_list_new(&status_list, repository, &Opts) != GIT_ERROR_NONE)
		return false;

	const size_t end = git_status_list_entrycount(status_list);
	for (size_t i = 0; i < end; ++i)
	{
		if (interrupt)
			return true;

		const auto* entry = git_status_byindex(status_list, i);
		if (entry->status & (GIT_STATUS_WT_NEW | GIT_STATUS_INDEX_NEW))
			added++;
		if (entry->status & (GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_TYPECHANGE | GIT_STATUS_WT_RENAMED | GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_INDEX_RENAMED | GIT_STATUS_INDEX_TYPECHANGE))
			modified++;
		if (entry->status & (GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_DELETED))
			deleted++;
	}

	return true;
}

bool GitLibLock::HasIncoming()
{
	if (!head)
		if (!GetHead())
			return false;

	if (!git_reference_is_branch(head))
		return false;

	git_oid local_oid, remote_oid;
	const auto branch = git_reference_shorthand(head);
	const auto remote_branch = std::string{ "refs/remotes/origin/" } + branch;

	if(git_reference_name_to_id(&local_oid, repository, branch) &&
		git_reference_name_to_id(&local_oid, repository, remote_branch.c_str()))
	{
		size_t ahead, behind;
		if(git_graph_ahead_behind(&ahead, &behind, repository, &local_oid, &remote_oid) == 0)
			return behind != 0;
	}

	return false;
}

bool GitLibLock::GetHead()
{
	return repository && git_repository_head(&head, repository) == GIT_ERROR_NONE;
}

bool GitLibLock::GetCurrentIndex()
{
	return repository && git_repository_index(&index, repository) == GIT_ERROR_NONE;
}
