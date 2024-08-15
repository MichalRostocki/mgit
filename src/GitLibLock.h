#pragma once
#include <string_view>

// Git2 references
// ReSharper disable CppInconsistentNaming
struct git_reference;
struct git_repository;
struct git_status_list;
// ReSharper restore CppInconsistentNaming

class GitLibLock
{
public:
	struct GitBranchStatus;
	struct GitFileStats;

	GitLibLock();
	~GitLibLock();

	bool OpenRepo(const std::string_view& path);
	
	GitBranchStatus GetBranchData();
	GitFileStats GetFileModificationStats();

	GitLibLock(const GitLibLock& other) = delete;
	GitLibLock(GitLibLock&& other) noexcept = delete;
	GitLibLock& operator=(const GitLibLock& other) = delete;
	GitLibLock& operator=(GitLibLock&& other) noexcept = delete;

private:
	git_repository* repository = nullptr;
	git_reference* head = nullptr;
	git_status_list* status_list = nullptr;

	bool GetHead();

public:

	struct GitBranchStatus
	{
		std::string branch_or_sha;
		bool failed = true;
		bool is_detached = false;
	};

	struct GitFileStats
	{
		bool failed = true;

		size_t added = 0;
		size_t modified = 0;
		size_t deleted = 0;
	};
};

