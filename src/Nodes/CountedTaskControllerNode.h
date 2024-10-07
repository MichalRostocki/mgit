#pragma once
#include <atomic>
#include <cstdint>

#include "ControllerNode.h"

class Task;

class CountedTaskControllerNode final : public ControllerNode
{
public:
	explicit CountedTaskControllerNode(Task* task, uint64_t expected_count);
	void Notify(const std::string& notify_data) override;

private:
	Task* task;
	std::atomic<uint64_t> expected_count;
};
