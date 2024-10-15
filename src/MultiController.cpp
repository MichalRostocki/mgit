#include "MultiController.h"

#include <fstream>
#include <iostream>

#include "Config.h"
#include "RepoOrchestrator.h"
#include "Data/Data.h"
#include "Displays/PipelineDisplay.h"
#include "Displays/StatusDisplay.h"
#include "Tasks/CommandTask.h"

// ReSharper disable once StringLiteralTypo
constexpr auto ConfigFilePathAppendix = R"(\.config\mgit\repos.json)";

namespace OutputControl
{
	void ClearLine(std::ostream& output_stream)
	{
		output_stream << "\33[2K\r";
	}

	void MoveCursorUp(std::ostream& output_stream, const size_t lines)
	{
		output_stream << "\33[" << lines << "A";
	}

	void MoveCursorDown(std::ostream& output_stream, const size_t lines)
	{
		output_stream << "\33[" << lines << "B";
	}
}

bool MultiController::LoadConfig(std::ostream& error_stream)
{
	// ReSharper disable once StringLiteralTypo
	// ReSharper disable once CppDeprecatedEntity
	const auto user_profile = getenv("USERPROFILE");
	const auto config_file_path = std::string{user_profile} + ConfigFilePathAppendix;

	std::ifstream f{config_file_path};
	if (!f.is_open())
	{
		error_stream << "Could not read config or it's empty. "
			<< "Localization: " << config_file_path;
		return false;
	}

	const nlohmann::json data = nlohmann::json::parse(f);
	f.close();

	config = data.get<Config>();

	return config.Validate();
}

int MultiController::DisplayStatus()
{

	std::function<void(const RepoConfig&, size_t)> register_function = [&register_function, this
		](const RepoConfig& repo_config, size_t sub_level)
	{
		if (!repo_config.hidden)
		{
			auto orchestrator = std::make_unique<RepoOrchestrator>(repo_config, sub_level);
			orchestrator->PlanStatusJob();
			tasks.Emplace(repo_config.repo_name, std::move(orchestrator));

			for (const auto& sub_repo : repo_config.sub_repos)
				register_function(sub_repo, sub_level + 1);
		}
	};

	for (const auto& repo_config : config.repositories)
		register_function(repo_config, 0);

	StatusDisplay display(tasks);

	const int result = RunTask(display);
	tasks.Clear();
	return result;
}

int MultiController::Build()
{
	// Build tasks
	for (const auto& repo_config : config.repositories)
	{
		if (!repo_config.build.steps.empty())
		{
			auto& repo_data = tasks[repo_config.repo_name];
			if(!repo_data)
				repo_data = std::make_shared<RepoOrchestrator>(repo_config, 0);
			repo_data->PlanBuildJobs();
		}
	}

	// Chains between repositories
	for (const auto & data : tasks)
	{
		const auto& task = data.second;
		const auto& task_config = task->GetConfig();

		for (const auto& required_name : task_config.build.require)
		{
			auto required_it = tasks.Find(required_name);
			if(required_it != tasks.end())
			{
				const auto& required_task = required_it->second;
				required_task->Register(task.get());
			}
			else
			{
				task->Notify(required_name);
			}
		}
	}

	PipelineDisplay display(tasks);

	const auto result = RunTask(display);
	tasks.Clear();
	return result;
}

const RepoConfig* MultiController::GetRepo(const std::string_view& repo_name) const
{
	for (const auto& repo_config : config.repositories)
	{
		if (repo_config.repo_name == repo_name)
		{
			return &repo_config;
		}
	}

	return nullptr;
}

bool MultiController::ShouldExit() const
{
	bool is_all_complete = true;

	for (const auto& data : tasks)
	{
		if (data.second->HasError())
			return true;
		if (!data.second->IsComplete())
			is_all_complete = false;
	}

	return is_all_complete;
}

bool MultiController::HasError() const
{
	for (const auto& repo_tasks : tasks)
		if (repo_tasks.second->HasError())
			return true;
	return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
int MultiController::RunTask(Display& display)
{
	for (const auto& task : tasks)
		task.second->Launch();

	bool is_finished;
	size_t no_of_lines = 0;

	do
	{
		std::ostringstream temp_buffer;

		if (no_of_lines)
		{
			temp_buffer << "\33[" << no_of_lines << "A";
		}

		is_finished = ShouldExit();
		if (is_finished)
			for (const auto& repo_tasks : tasks)
				repo_tasks.second->Stop();

		no_of_lines = display.Print(temp_buffer, is_finished);

		std::cout << temp_buffer.str();

		if (!is_finished)
			std::this_thread::sleep_for(std::chrono::milliseconds{100});
	}
	while (!is_finished);

	return HasError() ? 1 : 0;
}
