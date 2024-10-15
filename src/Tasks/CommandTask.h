#pragma once
#include <string>

#include "Task.h"

class CommandTask final : public Task
{
public:
	explicit CommandTask(RepoOrchestrator* repo_orchestrator, StepData& step, std::string command);

	bool Run() override;
	std::string_view GetCommand() override;

private:
	std::string command;
};
