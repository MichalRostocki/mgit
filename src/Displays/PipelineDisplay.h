#pragma once
#include <chrono>

#include "Display.h"

class PipelineDisplay final : public Display
{
public:
	explicit PipelineDisplay(const MultiControllerTasks& data);
	size_t Print(std::ostream& output, bool will_exit) override;

private:
	const MultiControllerTasks& data_collection;

	size_t max_name_space = 0;
	size_t last_max_line = 0;
	std::chrono::system_clock::time_point start_point;
};
