#pragma once
#include <list>

#include "Display.h"

struct StatusData;

class StatusDisplay final : public Display
{
public:
	explicit StatusDisplay(const std::list<StatusData>& data);
	size_t Print(std::ostream& output, bool will_exit) override;

private:
	const std::list<StatusData>& data_collection;

	size_t max_name_space;
};
