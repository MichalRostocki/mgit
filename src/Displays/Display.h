#pragma once
#include <ostream>

template<typename Key, typename Value>
class OrderedMap;
class RepoOrchestrator;
using MultiControllerTasks = OrderedMap<std::string, std::shared_ptr<RepoOrchestrator>>;

class Display
{
public:
	virtual ~Display() = default;
	virtual size_t Print(std::ostream& output, bool will_exit) = 0;

	Display() = default;
	Display(const Display& other) = delete;
	Display(Display&& other) noexcept = delete;
	Display& operator=(const Display& other) = delete;
	Display& operator=(Display&& other) noexcept = delete;

protected:
	static constexpr const char* ClearSymbol = "\33[2K\r";
};
