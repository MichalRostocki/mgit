#pragma once
#include "Display.h"

class StatusDisplay final : public Display
{
public:
	explicit StatusDisplay(const MultiControllerTasks& data);
	size_t Print(std::ostream& output, bool will_exit) override;

private:
	const MultiControllerTasks& data_collection;

	size_t max_name_space;
};
