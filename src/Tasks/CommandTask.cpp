#include "CommandTask.h"

#include <filesystem>
#include <Windows.h>

#include "Config.h"
#include "Data/CommandData.h"

namespace
{
	void LaunchWindowsApp(int& callback, std::stringstream& app_output, std::string& error_log, const std::string& command, const std::filesystem::path& directory, const std::atomic<bool>& stop_flag)
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
			const_cast<char*>(command.c_str()), // Command line
			NULL,                // Process handle not inheritable
			NULL,                // Thread handle not inheritable
			TRUE,                // Set handle inheritance to FALSE
			0,                   // No creation flags
			// !env.empty() ? const_cast<char*>(env.c_str()) : NULL,
			NULL,
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
				if (stop_flag)
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
				while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
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
			error_log = "Failed to create process " + command;
		}
	}
}

CommandTask::CommandTask(const RepoConfig& config, CommandData& data) :
	Task(config),
	data(data)
{
}

void CommandTask::Run()
{
	data.initialized = true;

	std::filesystem::path working_dir = repo_config.path;
	working_dir.append(repo_config.build.working_dir);
	if (!exists(working_dir))
	{
		data.error = "Could not find directory " + working_dir.string();
		error_encountered = true;
		return;
	}

	TASK_RUNNER_CHECK;

	int callback = 255;
	std::string error_log;
	LaunchWindowsApp(callback, data.app_output, error_log, data.command, working_dir, should_stop);

	TASK_RUNNER_CHECK;

	if (callback != 0)
	{
		if (!error_log.empty())
			data.error = std::move(error_log);
		else
			data.error = "Command failed";

		error_encountered = true;
	}

	data.completed = true;
}
