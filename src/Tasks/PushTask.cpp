#include "PushTask.h"

#include "Config.h"
#include "RepoOrchestrator.h"

namespace
{
	constexpr auto PushCommand = "git push local ";
}

PushLocalTask::PushLocalTask(RepoOrchestrator* repo_orchestrator, StepData& step) :
	CommandTask(repo_orchestrator, step, PushCommand + repo_orchestrator->GetConfig().default_branch)
{
}

std::string_view PushLocalTask::GetCommand()
{
	return "Updating local repo";
}
