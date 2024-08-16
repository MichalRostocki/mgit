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

bool GitLibLock::GetBranchData(bool& is_detached, std::string& branch_or_sha)
{
	if (!repository)
		return false;

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

bool GitLibLock::GetHead()
{
	return git_repository_head(&head, repository) == GIT_ERROR_NONE;
}
