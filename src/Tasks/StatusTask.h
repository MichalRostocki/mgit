#pragma once
#include "Task.h"

struct StatusData;

class StatusTask final : public Task
{
public:
	explicit StatusTask(RepoOrchestrator* repo_orchestrator, StepData& step);

	bool Run() override;
	std::string_view GetCommand() override;
};
