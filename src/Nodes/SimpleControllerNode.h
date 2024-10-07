#pragma once
#include <functional>

#include "ControllerNode.h"

class SimpleControllerNode final : public ControllerNode
{
public:
	explicit SimpleControllerNode(const std::function<void(const std::string&)>& function);
	void Notify(const std::string& notify_data) override;

private:
	std::function<void(const std::string&)> function;
};
