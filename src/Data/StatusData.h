#pragma once
#include <string>

struct StatusData
{
	std::string repo_name;
	std::string default_branch;
	std::string current_branch;

	size_t sub_repo_level;
	size_t files_added = 0;
	size_t files_modified = 0;
	size_t files_deleted = 0;

	bool is_repo_found = true;
	bool is_repo_detached = false;
	bool no_of_files_complete = false;

	explicit StatusData(std::string repo_name, std::string default_branch, const size_t sub_repo_level) :
		repo_name(std::move(repo_name)),
		default_branch(std::move(default_branch)),
		sub_repo_level(sub_repo_level)
	{
	}
};
