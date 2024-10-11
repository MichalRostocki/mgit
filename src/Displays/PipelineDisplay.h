#pragma once
#include <chrono>
#include <map>

#include "Display.h"

struct RepoCommandData;

class PipelineDisplay final : public Display
{
public:
	explicit PipelineDisplay(const std::map<std::string, std::shared_ptr<RepoCommandData>>& data);
	size_t Print(std::ostream& output, bool will_exit) override;

private:
	const std::map<std::string, std::shared_ptr<RepoCommandData>>& data_collection;

	size_t max_name_space = 0;
	size_t last_max_line = 0;
	std::chrono::system_clock::time_point start_point;
};
