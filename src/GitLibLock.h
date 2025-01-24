#pragma once
#include <functional>
#include <string>

#include "Tasks/PullPrepareTask.h"

// Git2 references
// ReSharper disable CppInconsistentNaming
struct git_index;
struct git_reference;
struct git_remote;
struct git_repository;
struct git_status_list;
// ReSharper restore CppInconsistentNaming

class GitLibLock
{
public:
	GitLibLock();
	~GitLibLock();

	bool OpenRepo(const std::string_view& path);
	bool LookupRemote(const std::string_view& name);
	bool ConnectToRemote(const std::string_view& remote_name);
	bool ConnectToRemote(std::function<int(const char*)>& remote_text_callback,
		std::function<int(unsigned, unsigned, size_t)>& progress_callback,
		const std::string_view& remote_name);
	bool Fetch(std::function<int(const char*)>& remote_text_callback,
		std::function<int(unsigned, unsigned, size_t)>& progress_callback,
		const std::string_view& remote_name);
	bool Pull(const std::string_view& remote_name);

	bool FullCheckoutToIndex();

	bool GetBranchData(bool& is_detached, std::string& branch_or_sha);
	bool GetFileModificationStats(const bool& interrupt, size_t& added, size_t& modified, size_t deleted);
	bool HasIncoming();

	GitLibLock(const GitLibLock& other) = delete;
	GitLibLock(GitLibLock&& other) noexcept = delete;
	GitLibLock& operator=(const GitLibLock& other) = delete;
	GitLibLock& operator=(GitLibLock&& other) noexcept = delete;

private:
	git_repository* repository = nullptr;
	git_reference* head = nullptr;
	git_index* index = nullptr;
	git_status_list* status_list = nullptr;
	git_remote* remote = nullptr;

	bool GetHead();
	bool GetCurrentIndex();
};

