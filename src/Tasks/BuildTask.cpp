#include "BuildTask.h"

#include "../Config.h"

// ReSharper disable once IdentifierTypo
#define NOMINMAX
#include <mutex>
#include <Windows.h>

namespace
{
	void LaunchBuildStep(const std::filesystem::path& directory, const std::string& step, const std::string& env, const bool& stop_flag, int& callback, std::string& error_log, std::stringstream& app_output)
	{
		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
		HANDLE hRead, hWrite;

		if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
			std::stringstream str;
			str << "Failed to create pipe: " << GetLastError() << std::endl;
			error_log = str.str();
			return;
		}

		if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) {
			std::stringstream str;
			str << "Failed to set handle information: " << GetLastError() << std::endl;
			error_log = str.str();
			return;
		}

		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdOutput = hWrite;
		si.hStdError = hWrite;  // Optional: Also redirect stderr

		if (CreateProcess(
			NULL,                // Application name
			const_cast<char*>(step.c_str()), // Command line
			NULL,                // Process handle not inheritable
			NULL,                // Thread handle not inheritable
			TRUE,                // Set handle inheritance to FALSE
			0,                   // No creation flags
			!env.empty() ? const_cast<char*>(env.c_str()) : NULL,
			directory.string().c_str(),                // Use parent's starting directory 
			&si,                 // Pointer to STARTUPINFO structure
			&pi                  // Pointer to PROCESS_INFORMATION structure
			)) 
		{
			CloseHandle(hWrite);

			// Wait until child process exits
			DWORD status;
			do
			{
				if(stop_flag)
				{
					TerminateProcess(pi.hProcess, 1);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					CloseHandle(hRead);
					return;
				}

				status = WaitForSingleObject(pi.hProcess, 0);

				char buffer[4096];
				DWORD bytes_read;
				while(ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
					if (bytes_read > 0) {
						buffer[bytes_read] = '\0';
						app_output << buffer;
					}
					else {
						break;
					}

					if (stop_flag)
					{
						TerminateProcess(pi.hProcess, 1);
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						CloseHandle(hRead);
						return;
					}
				}

			} while (status == WAIT_TIMEOUT);

			DWORD exit_code;
			if (GetExitCodeProcess(pi.hProcess, &exit_code)) {
				callback = static_cast<int>(exit_code);
			}
			else {
				std::stringstream str;
				str << "Failed to get exit code: " << GetLastError();
				error_log = str.str();
			}

			// Close process and thread handles
			CloseHandle(hRead);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else {
			error_log = "Failed to create process " + step;
		}
	}
}

const BuildTask::TaskStatus* BuildTask::GetBuildCompleteNotifier(const std::string& required_repo) const
{
	const auto it = data.find(required_repo);
	if (it == data.end())
		return nullptr;

	return &it->second.status;
}

bool BuildTask::IsSuccessful()
{
	for (const auto & complete : data)
	{
		if (complete.second.status != TaskStatus::Finished)
			return false;
	}

	return true;
}

std::unique_ptr<Task::TaskRunner> BuildTask::RegisterRepo(const RepoConfig& repository)
{
	data.insert({ repository.repo_name, {} });
	return std::make_unique<BuildTaskRunner>(repository, this, &data[repository.repo_name]);
}

size_t BuildTask::Display(std::ostream& output_stream)
{
	size_t lines_written = 0;
	size_t current_max_line = 0;

	const std::pair<const std::string, BuildTaskData>* first_failed = nullptr;
	const std::pair<const std::string, BuildTaskData>* first_ongoing = nullptr;

	// iterate builds
	for (const std::pair<const std::string, BuildTaskData>& d : data)
	{
		const BuildTaskData& build_data = d.second;

		if(build_data.repo_config->build.steps.empty() && build_data.status != TaskStatus::Failed)
			continue;

		if (!first_failed && build_data.status == TaskStatus::Failed)
			first_failed = &d;

		if (!first_ongoing && build_data.status == TaskStatus::Ongoing)
			first_ongoing = &d;

		lines_written++;
		output_stream << ' ' << build_data.repo_config->repo_name;

		auto spaces_needed = max_name_space - build_data.repo_config->repo_name.size();
		for (size_t i = 0; i < spaces_needed; ++i)
			output_stream << ' ';

		auto stage = build_data.stage;
		if (stage.size() > 80)
			stage = stage.substr(0, 80);
		output_stream << stage;

		// cleanup
		size_t line_length = build_data.repo_config->repo_name.size() + stage.size();
		if (line_length < last_max_line)
		{
			spaces_needed = std::max(last_max_line - line_length, 0ull);
			for (size_t i = 0; i < spaces_needed; ++i)
				output_stream << ' ';
		}

		output_stream << std::endl;

		current_max_line = std::max(current_max_line, line_length);
	}

	// build is taking long, what's going on?
	if(first_ongoing && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_point).count() > 5)
	{
		std::string str = first_ongoing->second.last_app_output.str();
		const size_t len = str.length();
		str = str.substr(len > 100 ? len - 100 : 0);

		output_stream << std::endl
			<< Clear << "Currently running: " << first_ongoing->second.repo_config->repo_name << std::endl
			<< Clear << " - Stage: " << first_ongoing->second.stage << std::endl
			<< Clear << " -  Progress: " << std::endl
			<< Clear << "   " << str << std::endl
			<< Clear << std::endl
			<< Clear << std::endl
			<< Clear << std::endl;

		lines_written += std::ranges::count(str, '\n') + 8;
	}

	// Show failed build
	if(first_failed && ShouldExit())
	{
		output_stream << std::endl << "Output from failed repo: " << first_failed->second.repo_config->repo_name << ": " << std::endl;
		output_stream << first_failed->second.stage << std::endl;
		output_stream << first_failed->second.last_app_output.rdbuf() << std::endl << std::endl;
		output_stream << "Output above was from failed repo " << first_failed->second.repo_config->repo_name << std::endl;
	}

	last_max_line = current_max_line;
	return lines_written;
}

void BuildTask::OnAllReposRegistered()
{
	for (const auto& d : data)
		max_name_space = std::max(d.second.repo_config->repo_name.size(), max_name_space);
	max_name_space += 4;

	start_point = std::chrono::system_clock::now();
}

bool BuildTask::ShouldExit()
{
	bool res = true;

	for (const auto& d : data)
	{
		if (d.second.status == TaskStatus::Failed)
			return true;
		if (d.second.status == TaskStatus::Ongoing || d.second.status == TaskStatus::Waiting)
			res = false;
	}

	return res;
}

BuildTask::BuildTaskRunner::BuildTaskRunner(const RepoConfig& repository, const BuildTask* engine, BuildTaskData* data) :
	TaskRunner(repository),
	data(data),
	engine(engine)
{
	data->repo_config = &repository;
}

void BuildTask::BuildTaskRunner::Run()
{
	// Waiting for other builds
	for (const auto& require: repo_config.build.require)
	{
		data->status = TaskStatus::Waiting;

		TASK_RUNNER_CHECK

		const auto* notifier = engine->GetBuildCompleteNotifier(require);
		if(!notifier)
		{
			data->stage = "Failed to contact repo " + require;
			data->status = TaskStatus::NotRun;
			return;
		}

		TASK_RUNNER_CHECK

		data->stage = "Waiting for build in " + require + " to complete";

		while(*notifier == TaskStatus::Ongoing || *notifier == TaskStatus::Waiting)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
			TASK_RUNNER_CHECK
		}

		if(*notifier == TaskStatus::Failed || *notifier == TaskStatus::NotRun)
		{
			data->stage = "Failed because " + require + " failed";
			data->status = TaskStatus::NotRun;
			return;
		}
	}

	if(repo_config.build.steps.empty())
	{
		data->stage = "Complete, no steps found";
		data->status = TaskStatus::Finished;
		return;
	}

	data->status = TaskStatus::Ongoing;

	std::filesystem::path working_dir = repo_config.path;
	working_dir.append(repo_config.build.working_dir);
	if(!exists(working_dir))
	{
		data->stage = "Could not find directory " + working_dir.string();
		data->status = TaskStatus::NotRun;
		return;
	}

	TASK_RUNNER_CHECK

	// Build environment
	std::string environment;
	if(!data->repo_config->build.env.empty())
	{
		std::stringstream env_stream;

		auto system_env = GetEnvironmentStrings();

		const char* temp_ptr = system_env;
		std::string temp_string;
		do
		{
			temp_string = temp_ptr;
			env_stream << temp_string;
			temp_ptr += temp_string.size() + 1;
		} while (!temp_string.empty());
		FreeEnvironmentStrings(system_env);

		for (const auto& env : data->repo_config->build.env)
		{
			env_stream << ';' << env;
		}
		env_stream >> environment;
	}

	TASK_RUNNER_CHECK

	// Builds
	for (size_t i = 0; i < repo_config.build.steps.size(); ++i)
	{
		const auto& step = repo_config.build.steps[i];
		data->last_app_output.clear();

		std::stringstream str;
		str << "Building (" << i << " / " << repo_config.build.steps.size() << ") - " << step;
		data->stage = str.str();

		TASK_RUNNER_CHECK

		int callback = 255;
		std::string error_log;
		std::thread step_thread{ [&working_dir, &step, &callback, &error_log, &environment, this] {LaunchBuildStep(working_dir, step, environment, should_stop, callback, error_log, data->last_app_output); } };

		// task runner checks are conducted in separate thread now
		while(!step_thread.joinable())
			std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
		step_thread.join();

		TASK_RUNNER_CHECK

		if(callback != 0)
		{
			if(error_log.empty())
			{
				std::stringstream str2;
				str2 << "Failed to complete step " << i << ": " << step;
				data->stage = str2.str();
			}
			else
			{
				data->stage = std::move(error_log);
			}

			data->status = TaskStatus::Failed;
			return;
		}

		TASK_RUNNER_CHECK
	}

	// Complete
	data->stage = "Complete!";
	data->status = TaskStatus::Finished;
}
