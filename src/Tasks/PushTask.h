#pragma once
#include "CommandTask.h"

class PushLocalTask final : public CommandTask
{
public:
	explicit PushLocalTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	std::string_view GetCommand() override;
};
