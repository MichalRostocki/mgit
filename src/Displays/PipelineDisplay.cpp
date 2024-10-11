#include "PipelineDisplay.h"

#include "Config.h"
#include "Data/CommandData.h"

PipelineDisplay::PipelineDisplay(const std::map<std::string, std::shared_ptr<RepoCommandData>>& data) :
	data_collection(data)
{
	for (const auto& d : data)
		max_name_space = std::max(d.second->repo_config.repo_name.size(), max_name_space);
	max_name_space += 4;
	start_point = std::chrono::system_clock::now();
}

size_t PipelineDisplay::Print(std::ostream& output, bool will_exit)
{
	size_t lines_written = 0;
	size_t current_max_line = 0;

	const std::pair<const std::string, std::shared_ptr<RepoCommandData>>* first_failed = nullptr;
	const std::pair<const std::string, std::shared_ptr<RepoCommandData>>* first_ongoing = nullptr;

	// iterate builds
	for (const auto& d : data_collection)
	{
		const auto& repo_data = d.second;
		const CommandData* active = repo_data->GetActive();

		if (!first_ongoing && active)
			first_ongoing = &d;
		
		if (!first_failed && active && !active->error.empty())
			first_failed = &d;
		
		lines_written++;
		output << ' ' << repo_data->repo_config.repo_name;
		
		auto spaces_needed = max_name_space - repo_data->repo_config.repo_name.size();
		for (size_t i = 0; i < spaces_needed; ++i)
			output << ' ';

		std::stringstream stage_stream;
		if (active)
			stage_stream << "Building (" << active->id << " / " << repo_data->steps_data.size() << ") - " << active->command;
		else if (repo_data->await_list.empty())
			stage_stream << "Completed!";
		else
			stage_stream << "Awaiting: " << *repo_data->await_list.begin();

		std::string stage = stage_stream.str();
		if (stage.size() > 80)
			stage = stage.substr(0, 80);
		output << stage;
		
		// cleanup
		size_t line_length = repo_data->repo_config.repo_name.size() + stage.size();
		if (line_length < last_max_line)
		{
			spaces_needed = std::max(last_max_line - line_length, 0ull);
			for (size_t i = 0; i < spaces_needed; ++i)
				output << ' ';
		}
		
		output << std::endl;
		
		current_max_line = std::max(current_max_line, line_length);
	}

	// build is taking long, what's going on?
	if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_point).count() > 5)
	{
		if (first_ongoing)
		{
			const auto* active = first_ongoing->second->GetActiveOrLast();
			std::string str = active->app_output.str();
			const size_t len = str.length();
			str = str.substr(len > 100 ? len - 100 : 0);

			std::stringstream stage_stream;
			if (!active->completed)
				stage_stream << "Building (" << active->id << " / " << first_ongoing->second->repo_config.build.steps.size() << ") - " << active->command;
			else stage_stream << "Completed!";
			std::string stage = stage_stream.str();
	
			output << std::endl
				<< Clear << "Currently running: " << first_ongoing->second->repo_config.repo_name << std::endl
				<< Clear << " - Stage: " << stage << std::endl
				<< Clear << " -  Progress: " << std::endl
				<< Clear << "   " << str << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl;
	
			lines_written += std::ranges::count(str, '\n') + 8;
		}
		else
		{
			output << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl;
		}
	}

	// Show failed build
	if (first_failed && will_exit)
	{
		const auto* active = first_failed->second->GetActiveOrLast();
		std::stringstream stage_stream;
		stage_stream << "Building (" << active->id << " / " << first_failed->second->repo_config.build.steps.size() << ") - " << active->command;
		std::string stage = stage_stream.str();

		output << std::endl << "Output from failed repo: " << first_failed->second->repo_config.repo_name << ": " << std::endl;
		output << stage << std::endl;
		output << active->app_output.rdbuf() << std::endl << std::endl;
		output << "Output above was from failed repo " << first_failed->second->repo_config.repo_name << std::endl;
	}

	last_max_line = current_max_line;
	return lines_written;
}
