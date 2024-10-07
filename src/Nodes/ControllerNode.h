#pragma once
#include <string>

class ControllerNode
{
public:
	virtual ~ControllerNode() = default;
	virtual void Notify(const std::string& notify_data) = 0;

	ControllerNode() = default;
	ControllerNode(const ControllerNode& other) = delete;
	ControllerNode(ControllerNode&& other) noexcept = delete;
	ControllerNode& operator=(const ControllerNode& other) = delete;
	ControllerNode& operator=(ControllerNode&& other) noexcept = delete;
};
