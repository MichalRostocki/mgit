#include "GitLibLock.h"

#include <git2.h>

GitLibLock::GitLibLock()
{
	git_libgit2_init();
}

GitLibLock::~GitLibLock()
{
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

GitLibLock::GitBranchStatus GitLibLock::GetBranchData()
{
	if (!repository)
		return {};

	if (!head)
		if (!GetHead())
			return {};

	GitBranchStatus result;
	result.is_detached = !git_reference_is_branch(head);
	result.failed = false;

	if (result.is_detached)
	{
		char sha[GIT_OID_SHA1_HEXSIZE + 1];
		const git_oid* oid = git_reference_target(head);
		git_oid_tostr(sha, sizeof(sha), oid);
		result.branch_or_sha = sha;
	}
	else
	{
		result.branch_or_sha = git_reference_shorthand(head);
	}

	return result;
}

GitLibLock::GitFileStats GitLibLock::GetFileModificationStats()
{
	constexpr static git_status_options Opts = GIT_STATUS_OPTIONS_INIT;

	if (!repository)
		return {};

	if (git_status_list_new(&status_list, repository, &Opts) != GIT_ERROR_NONE)
		return {};

	GitFileStats result;

	const size_t end = git_status_list_entrycount(status_list);
	for (size_t i = 0; i < end; ++i)
	{
		const auto* entry = git_status_byindex(status_list, i);
		if (entry->status & GIT_STATUS_WT_NEW)
			result.added++;
		if (entry->status & (GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_TYPECHANGE | GIT_STATUS_WT_RENAMED))
			result.modified++;
		if (entry->status & GIT_STATUS_WT_DELETED)
			result.deleted++;
	}

	result.failed = false;
	return result;
}

bool GitLibLock::GetHead()
{
	return git_repository_head(&head, repository) == GIT_ERROR_NONE;
}
