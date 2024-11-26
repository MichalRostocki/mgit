#include "CommandTask.h"

#include <filesystem>
#include <Windows.h>

#include "Config.h"
#include "RepoOrchestrator.h"

namespace
{
	void LaunchWindowsApp(int& callback, std::stringstream& app_output, std::string& error_log,
	                      const std::string& command, const std::filesystem::path& directory,
	                      const std::atomic<bool>& stop_flag)
	{
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi;
		SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
		HANDLE h_read, h_write;

		if (!CreatePipe(&h_read, &h_write, &sa, 0))
		{
			std::stringstream str;
			str << "Failed to create pipe: " << GetLastError() << std::endl;
			error_log = str.str();
			return;
		}

		if (!SetHandleInformation(h_read, HANDLE_FLAG_INHERIT, 0))
		{
			std::stringstream str;
			str << "Failed to set handle information: " << GetLastError() << std::endl;
			error_log = str.str();
			return;
		}

		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdOutput = h_write;
		si.hStdError = h_write;

		if (CreateProcess(
			nullptr,
			const_cast<char*>(command.c_str()),
			nullptr,
			nullptr,
			TRUE,
			0,
			nullptr,
			directory.string().c_str(),
			&si,
			&pi
		))
		{
			CloseHandle(h_write);

			// Wait until child process exits
			DWORD status;
			do
			{
				if (stop_flag)
				{
					TerminateProcess(pi.hProcess, 1);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					CloseHandle(h_read);
					return;
				}

				status = WaitForSingleObject(pi.hProcess, 0);

				char buffer[4096];
				DWORD bytes_read;
				while (ReadFile(h_read, buffer, sizeof(buffer) - 1, &bytes_read, nullptr))
				{
					if (bytes_read > 0)
					{
						buffer[bytes_read] = '\0';
						app_output << buffer;
					}
					else
					{
						break;
					}

					if (stop_flag)
					{
						TerminateProcess(pi.hProcess, 1);
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						CloseHandle(h_read);
						return;
					}
				}
			}
			while (status == WAIT_TIMEOUT);

			DWORD exit_code;
			if (GetExitCodeProcess(pi.hProcess, &exit_code))
			{
				callback = static_cast<int>(exit_code);
			}
			else
			{
				std::stringstream str;
				str << "Failed to get exit code: " << GetLastError();
				error_log = str.str();
			}

			// Close process and thread handles
			CloseHandle(h_read);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			error_log = "Failed to create process " + command;
		}
	}
}

CommandTask::CommandTask(RepoOrchestrator* repo_orchestrator, StepData& step, std::string command) :
	Task(repo_orchestrator, step),
	command(std::move(command))
{
}

bool CommandTask::Run()
{
	const auto& config = GetConfig();

	std::filesystem::path working_dir = config.path;
	working_dir.append(config.build.working_dir);
	if (!exists(working_dir))
	{
		step_data.error = "Could not find directory " + working_dir.string();
		return false;
	}

	TASK_RUNNER_CHECK;

	int callback = 255;
	std::string error_log;
	LaunchWindowsApp(callback, step_data.output, error_log, command, working_dir, should_stop);

	TASK_RUNNER_CHECK;

	if (callback != 0)
	{
		if (!error_log.empty())
			step_data.error = std::move(error_log);
		else
			step_data.error = "Command failed";

		return false;
	}

	return true;
}

std::string_view CommandTask::GetCommand()
{
	return command;
}
