#pragma once
#include "Task.h"

struct CommandData;

class CommandTask final : public Task
{
public:
	explicit CommandTask(const RepoConfig& config, CommandData& data);

private:
	CommandData& data;

	void Run() override;
};
