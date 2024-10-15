#include "PipelineDisplay.h"

#include "Config.h"
#include "RepoOrchestrator.h"
#include "OrderedMap.h"

PipelineDisplay::PipelineDisplay(const MultiControllerTasks& data) :
	data_collection(data)
{
	for (const auto& d : data)
		max_name_space = std::max(d.second->GetConfig().repo_name.size(), max_name_space);
	max_name_space += 4;
	start_point = std::chrono::system_clock::now();
}

size_t PipelineDisplay::Print(std::ostream& output, bool will_exit)
{
	size_t lines_written = 0;
	size_t current_max_line = 0;

	const std::pair<const std::string, std::shared_ptr<RepoOrchestrator>>* first_failed = nullptr;
	const std::pair<const std::string, std::shared_ptr<RepoOrchestrator>>* first_ongoing = nullptr;

	// iterate builds
	for (const auto& d : data_collection)
	{
		const auto& repo_data = d.second;
		const auto repo_status = repo_data->GetCurrentStatus();
		const auto repo_config = repo_data->GetConfig();

		if (!first_ongoing && repo_status == OrchestratorStatus::Ongoing)
			first_ongoing = &d;
		
		if (!first_failed && repo_status == OrchestratorStatus::Error)
			first_failed = &d;
		
		lines_written++;
		output << ' ' << repo_config.repo_name;
		
		auto spaces_needed = max_name_space - repo_config.repo_name.size();
		for (size_t i = 0; i < spaces_needed; ++i)
			output << ' ';

		std::stringstream stage_stream;
		switch(repo_status)
		{
		case OrchestratorStatus::Awaiting:
			stage_stream << "Awaiting: " << repo_data->GetAwait();
			break;
		case OrchestratorStatus::Ongoing:
			stage_stream << "Building (" << repo_data->GetActiveId() << " / " << repo_data->GetSize() << ") - " << repo_data->GetActiveCommand();
			break;
		case OrchestratorStatus::Complete:
			stage_stream << "Completed!";
			break;
		case OrchestratorStatus::Error:
			stage_stream << "Error encountered!";
			break;
		}

		std::string stage = stage_stream.str();
		if (stage.size() > 80)
			stage = stage.substr(0, 80);
		output << stage;
		
		// cleanup
		size_t line_length = repo_config.repo_name.size() + stage.size();
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
			const auto& repo_data = first_ongoing->second;
			std::string str = repo_data->GetActiveOutput();
			const size_t len = str.length();
			str = str.substr(len > 100 ? len - 100 : 0);

			std::stringstream stage_stream;
			stage_stream << "Building (" << repo_data->GetActiveId() << " / " << repo_data->GetSize() << ") - " << repo_data->GetActiveCommand();
			std::string stage = stage_stream.str();
	
			output << std::endl
				<< Clear << "Currently running: " << repo_data->GetConfig().repo_name << std::endl
				<< Clear << " - Stage: " << stage << std::endl
				<< Clear << " -  Progress: " << std::endl
				<< Clear << "   " << str << std::endl
				<< Clear << std::endl
				<< Clear << std::endl
				<< Clear << std::endl;
	
			lines_written += std::ranges::count(str, '\n') + 9;
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
		const auto& repo_data = first_failed->second;
		const auto& repo_config = repo_data->GetConfig();

		output << std::endl << "Output from failed repo: " << repo_config.repo_name << ": " << std::endl;
		output << "Building (" << repo_data->GetActiveId() << " / " << repo_data->GetSize() << ") - " << repo_data->GetActiveCommand() << std::endl;
		output << "Error: " << repo_data->GetErrorString() << std::endl;
		output << repo_data->GetActiveOutput() << std::endl << std::endl;
		output << "Output above was from failed repo " << repo_config.repo_name << std::endl;
	}

	last_max_line = current_max_line;
	return lines_written;
}
