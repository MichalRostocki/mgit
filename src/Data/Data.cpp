#include "Data.h"
#include "CommandData.h"

CommandData::CommandData(const size_t id, const std::string& command, std::string environment) :
	id(id),
	command(command),
	environment(std::move(environment))
{
}

RepoCommandData::RepoCommandData(const RepoConfig& repo_config):
	repo_config(repo_config)
{
}

const CommandData* RepoCommandData::GetActiveOrLast() const
{
	if (!steps_data.front()->initialized)
		return nullptr;
	for (const auto& command_data : steps_data)
		if (!command_data->completed || !command_data->error.empty())
			return command_data.get();
	return steps_data.back().get();
}

const CommandData* RepoCommandData::GetActive() const
{
	if (!steps_data.front()->initialized)
		return nullptr;
	for (int i = static_cast<int>(steps_data.size()) - 1; i >= 0; --i)
	{
		const auto& command_data = steps_data[i];
		if ((!command_data->completed && command_data->initialized) || !command_data->error.empty())
			return command_data.get();
	}
	return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void RepoCommandData::SkipToTask(const size_t size)
{
	for (size_t i = 0; i < size; ++i)
	{
		if (!steps_data[i]->completed)
		{
			steps_data[i]->initialized = true;
			steps_data[i]->completed = true;
		}
	}
}
