#pragma once
#include <atomic>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct RepoConfig;

struct CommandData
{
	const size_t id;
	const std::string& command;
	const std::string environment;

	std::atomic<bool> initialized{ false };
	std::atomic<bool> completed{ false };

	std::string error;
	std::stringstream app_output;

	CommandData(const size_t id, const std::string& command, std::string environment) :
		id(id),
		command(command),
		environment(std::move(environment))
	{
	}
};

struct RepoCommandData
{
	const RepoConfig& repo_config;
	std::set<std::string> await_list;
	std::vector<std::unique_ptr<CommandData>> steps_data;

	explicit RepoCommandData(const RepoConfig& repo_config) :
		repo_config(repo_config)
	{
	}

	const CommandData* GetActiveOrLast() const
	{
		if (!steps_data.front()->initialized)
			return nullptr;
		for (const auto& command_data : steps_data)
			if (!command_data->completed || !command_data->error.empty())
				return command_data.get();
		return steps_data.back().get();
	}

	const CommandData* GetActive() const
	{
		if (!steps_data.front()->initialized)
			return nullptr;
		for (const auto& command_data : steps_data)
			if (!command_data->completed || !command_data->error.empty())
				return command_data.get();
		return nullptr;
	}
};
