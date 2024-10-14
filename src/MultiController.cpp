#include "MultiController.h"

#include <fstream>
#include <iostream>

#include "Config.h"
#include "Data/CommandData.h"
#include "Data/Data.h"
#include "Data/StatusData.h"
#include "Displays/PipelineDisplay.h"
#include "Displays/StatusDisplay.h"
#include "Nodes/CountedTaskControllerNode.h"
#include "Nodes/SimpleControllerNode.h"
#include "Tasks/CommandTask.h"
#include "Tasks/StatusTask.h"

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

namespace
{
	void AddCommandTasks(const RepoConfig& repo_config, const std::vector<std::string>& steps, const std::shared_ptr<RepoTaskCollection>& output_tasks, const std::shared_ptr<RepoCommandData>& repo_data)
	{
		if (!steps.empty())
		{
			std::string environment;
			for (const auto& step : steps)
			{
				auto current_data = std::make_unique<CommandData>(output_tasks->tasks.size(), step, environment);
				auto task = std::make_unique<CommandTask>(repo_config, *current_data);

				repo_data->steps_data.push_back(std::move(current_data));
				output_tasks->tasks.push_back(std::move(task));
			}
		}
	}

	void ConnectTasks(const std::shared_ptr<RepoTaskCollection>& repo_tasks, const size_t start_index, const size_t end_index)
	{
		const auto task_finished_notifier = repo_tasks->GetFinishNotifier();
		for (size_t i = start_index; i < end_index; ++i)
		{
			const auto& task = repo_tasks->tasks[i];

			// Notify RepoData that task is complete on end
			task->NotifyOnEnd(task_finished_notifier);

			// Attach to previous task in given repository
			if (i != start_index)
				repo_tasks->tasks[i - 1]->NotifyOnEnd(task->GetSimpleNotifier());
		}
	}

	void ConnectTasks(const std::shared_ptr<RepoTaskCollection>& repo_tasks)
	{
		ConnectTasks(repo_tasks, 0, repo_tasks->tasks.size());
	}

	void ApplyErrorHandling(const RepoConfig& repo_config, const std::shared_ptr<RepoTaskCollection>& repo_tasks, const std::shared_ptr<RepoCommandData>& repo_data)
	{
		const auto basic_tasks_count = repo_config.build.steps.size();
		const auto before_retry_tasks_count = repo_config.build.on_error.before_retry.size();
		const auto all_tasks_count = basic_tasks_count + (repo_config.build.on_error.retry ? basic_tasks_count + before_retry_tasks_count : 0);

		const auto fatal_error_handler = repo_tasks->GetErrorNotifier();

		if (repo_config.build.on_error.retry)
		{
			if (!repo_config.build.on_error.before_retry.empty())
			{
				AddCommandTasks(repo_config, repo_config.build.on_error.before_retry, repo_tasks, repo_data);
			}

			AddCommandTasks(repo_config, repo_config.build.steps, repo_tasks, repo_data);
			ConnectTasks(repo_tasks, basic_tasks_count, all_tasks_count);

			auto* retry_task = repo_tasks->tasks[basic_tasks_count].get();
			const auto soft_error_handler = std::make_shared<SimpleControllerNode>(
			[basic_tasks_count, repo_data, repo_tasks, retry_task](const std::string&)
			{
				repo_data->SkipToTask(basic_tasks_count);
				repo_tasks->current_task_index = basic_tasks_count;
				retry_task->Launch();
			});

			for (size_t i = 0; i < basic_tasks_count; ++i)
				repo_tasks->tasks[i]->NotifyOnError(soft_error_handler);

			for (size_t i = basic_tasks_count; i < repo_tasks->tasks.size(); ++i)
				repo_tasks->tasks[i]->NotifyOnError(fatal_error_handler);

			auto* last_task = repo_tasks->tasks.back().get();
			const auto non_error_finisher = std::make_shared<SimpleControllerNode>([last_task, repo_data, repo_tasks](const std::string&)
			{
				last_task->InvokeSuccess();
				repo_data->SkipToTask(repo_data->steps_data.size());
				repo_tasks->current_task_index = repo_data->steps_data.size();
			});
			repo_tasks->tasks[basic_tasks_count - 1]->NotifyOnEnd(non_error_finisher);
		}
		else
		{
			for (const auto& task : repo_tasks->tasks)
				task->NotifyOnError(fatal_error_handler);
		}
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
	std::list<Task*> default_tasks;
	std::list<StatusData> data;

	std::function<void(const RepoConfig&, size_t)> register_function = [&default_tasks, &data, &register_function, this
		](const RepoConfig& repo_config, size_t sub_level)
	{
		if (!repo_config.hidden)
		{
			auto& repo_tasks = *tasks.emplace(repo_config.repo_name, std::make_unique<RepoTaskCollection>()).first->
			                          second;
			auto& current_data = data.emplace_back(repo_config.repo_name, repo_config.default_branch, sub_level);
			auto task = std::make_unique<StatusTask>(repo_config, current_data);

			task->NotifyOnEnd(repo_tasks.GetFinishNotifier());
			task->NotifyOnError(repo_tasks.GetErrorNotifier());

			default_tasks.push_back(&*task);
			repo_tasks.tasks.push_back(std::move(task));


			for (const auto& sub_repo : repo_config.sub_repos)
				register_function(sub_repo, sub_level + 1);
		}
	};

	for (const auto& repo_config : config.repositories)
		register_function(repo_config, 0);

	StatusDisplay display(data);

	const int result = RunTask(default_tasks, display);
	tasks.clear();
	data.clear();
	return result;
}

int MultiController::Build()
{
	std::map<std::string, std::shared_ptr<RepoCommandData>> data;

	// Build tasks
	for (const auto& repo_config : config.repositories)
	{
		if (!repo_config.build.steps.empty())
		{
			const size_t expected_tasks = repo_config.build.steps.size()
				+ (repo_config.build.on_error.retry ? repo_config.build.steps.size() + repo_config.build.on_error.before_retry.size() : 0);

			auto repo_tasks = std::make_shared<RepoTaskCollection>();
			repo_tasks->tasks.reserve(expected_tasks);

			auto repo_data = std::make_shared<RepoCommandData>(repo_config);
			repo_data->await_list.insert(repo_config.build.require.begin(), repo_config.build.require.end());
			repo_data->steps_data.reserve(expected_tasks);

			AddCommandTasks(repo_config, repo_config.build.steps, repo_tasks, repo_data);
			ConnectTasks(repo_tasks);
			ApplyErrorHandling(repo_config, repo_tasks, repo_data);

			tasks[repo_config.repo_name] = std::move(repo_tasks);
			data[repo_config.repo_name] = std::move(repo_data);
		}
	}

	// Chains between repositories
	std::list<Task*> default_tasks;

	for (const auto& repo_config : config.repositories)
	{
		if (!repo_config.build.steps.empty())
		{
			auto& repo_data = data[repo_config.repo_name];

			std::list<Task*> required_tasks;
			for (const auto& requirement : repo_config.build.require)
			{
				auto it = tasks.find(requirement);
				if (it != tasks.end())
					required_tasks.push_back(it->second->tasks.back().get());
			}

			auto* first_task = tasks.at(repo_config.repo_name)->tasks[0].get();
			if (required_tasks.empty())
			{
				default_tasks.push_back(first_task);
			}
			else
			{
				auto node = std::make_shared<CountedTaskControllerNode>(first_task, required_tasks.size());
				auto data_node = std::make_shared<SimpleControllerNode>([&repo_data](const std::string& s)
				{
					repo_data->await_list.erase(s);
				});
				for (auto* required_task : required_tasks)
				{
					required_task->NotifyOnEnd(node);
					required_task->NotifyOnEnd(data_node);
				}
			}
		}
	}

	PipelineDisplay display(data);

	const auto result = RunTask(default_tasks, display);

	tasks.clear();
	data.clear();

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

	for (const auto& repo_tasks : tasks)
	{
		if (repo_tasks.second->error_encountered)
			return true;
		if (!repo_tasks.second->IsComplete())
			is_all_complete = false;
	}

	return is_all_complete;
}

bool MultiController::HasError() const
{
	for (const auto& repo_tasks : tasks)
		if (repo_tasks.second->error_encountered)
			return true;
	return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
int MultiController::RunTask(const std::list<Task*>& default_tasks, Display& display)
{
	for (const auto& task : default_tasks)
		task->Launch();

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

std::shared_ptr<ControllerNode> RepoTaskCollection::GetFinishNotifier()
{
	return std::make_shared<SimpleControllerNode>([this](const std::string& _)
	{
		++current_task_index;
	});
}

std::shared_ptr<ControllerNode> RepoTaskCollection::GetErrorNotifier()
{
	return std::make_shared<SimpleControllerNode>([this](const std::string& _)
	{
		error_encountered = true;
	});
}

bool RepoTaskCollection::IsComplete() const
{
	return current_task_index >= tasks.size();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void RepoTaskCollection::Stop()
{
	for (const auto& task : tasks)
		task->ForceStop();
}
