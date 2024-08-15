#pragma once
#include <git2/types.h>

struct GitLibLock
{
	GitLibLock();
	~GitLibLock();

	GitLibLock(const GitLibLock& other) = delete;
	GitLibLock(GitLibLock&& other) noexcept = delete;
	GitLibLock& operator=(const GitLibLock& other) = delete;
	GitLibLock& operator=(GitLibLock&& other) noexcept = delete;

	git_repository* repository = nullptr;
	git_reference* head = nullptr;
};

