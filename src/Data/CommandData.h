#pragma once
#include <atomic>
#include <sstream>
#include <string>

struct CommandData
{
	const size_t id;
	const std::string& command;
	const std::string environment;

	std::atomic<bool> initialized{ false };
	std::atomic<bool> completed{ false };

	std::string error;
	std::stringstream app_output;

	CommandData(size_t id, const std::string& command, std::string environment);
};
