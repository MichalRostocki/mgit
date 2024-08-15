#pragma once
#include <string_view>

// Git2 references
// ReSharper disable CppInconsistentNaming
struct git_reference;
struct git_repository;
// ReSharper restore CppInconsistentNaming

class GitLibLock
{
public:
	struct GitBranchStatus;

	GitLibLock();
	~GitLibLock();

	bool OpenRepo(const std::string_view& path);
	
	GitBranchStatus GetBranchData();

	GitLibLock(const GitLibLock& other) = delete;
	GitLibLock(GitLibLock&& other) noexcept = delete;
	GitLibLock& operator=(const GitLibLock& other) = delete;
	GitLibLock& operator=(GitLibLock&& other) noexcept = delete;

private:
	git_repository* repository = nullptr;
	git_reference* head = nullptr;

	bool GetHead();

public:

	struct GitBranchStatus
	{
		std::string branch_or_sha;
		bool failed = true;
		bool is_detached = false;
	};
};

