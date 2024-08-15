#include "Task.h"

#include <ostream>

#include "../Config.h"

Task::~Task()
{
    StopProcedure();
}

void Task::Register(const Config& config)
{
	for (const auto& repo_config : config.Repositories)
	{
		if (auto runner = RegisterRepo(repo_config))
			runners.emplace_back(std::move(runner));
	}
}

void Task::Process(std::ostream& output_stream)
{
    OnAllReposRegistered();

	for (const auto& runner : runners)
		threads.emplace_back([&runner] { runner->Run(); });

	std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });

    bool is_finished;
    size_t no_of_lines = 0;

    do
    {
        if(no_of_lines)
        {
            output_stream << "\33[" << no_of_lines << "A";
        }

		is_finished = ShouldExit();
        no_of_lines = Display(output_stream);

        if (!is_finished)
            std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });

    } while (!is_finished);
}

Task::TaskRunner::TaskRunner(const RepoConfig& repo_config) :
    repo_config(repo_config),
    repo_name(std::filesystem::path{ repo_config.Path }.filename().string())
{
}

void Task::TaskRunner::Stop()
{
    should_stop = true;
}

std::string_view Task::TaskRunner::GetRepoName() const
{
    return repo_name;
}

void Task::ClearCurrentLine(std::ostream& output_stream)
{
    output_stream << "\33[2K\r";
}

void Task::StopProcedure()
{
	for (const auto& task_runner : runners)
        task_runner->Stop();

    // Wait for everyone to get finished

    constexpr auto max_step = 100;
    auto step = 0;
    bool all_joined;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 2 * step });
        all_joined = true;

        for (const auto& thread : threads)
            if (!thread.joinable())
            {
                all_joined = false;
                break;
            }

        step++;

    } while (!all_joined && max_step != step);

	for (auto & thread : threads)
        thread.join();

    threads.clear();
    runners.clear();
}