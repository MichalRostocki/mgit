#pragma once
#include "ControllerNode.h"

class Task;

class SimpleTaskControllerNode final : public ControllerNode
{
public:
	explicit SimpleTaskControllerNode(Task* task);
	void Notify(const std::string& notify_data) override;

private:
	Task* task;
};
