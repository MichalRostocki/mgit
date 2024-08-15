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

	if (repository)
		git_repository_free(repository);

	git_libgit2_shutdown();
}
