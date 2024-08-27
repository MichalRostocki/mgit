#pragma once
#include <string>

// Git2 references
// ReSharper disable CppInconsistentNaming
struct git_reference;
struct git_repository;
struct git_status_list;
// ReSharper restore CppInconsistentNaming

class GitLibLock
{
public:
	GitLibLock();
	~GitLibLock();

	bool OpenRepo(const std::string_view& path);
	
	bool GetBranchData(bool& is_detached, std::string& branch_or_sha);
	bool GetFileModificationStats(const bool& interrupt, size_t& added, size_t& modified, size_t deleted);

	GitLibLock(const GitLibLock& other) = delete;
	GitLibLock(GitLibLock&& other) noexcept = delete;
	GitLibLock& operator=(const GitLibLock& other) = delete;
	GitLibLock& operator=(GitLibLock&& other) noexcept = delete;

private:
	git_repository* repository = nullptr;
	git_reference* head = nullptr;
	git_status_list* status_list = nullptr;

	bool GetHead();
};

