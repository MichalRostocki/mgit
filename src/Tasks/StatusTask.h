#pragma once
#include "Task.h"

struct StatusData;

class StatusTask final : public Task
{
public:
	explicit StatusTask(const RepoConfig& config, StatusData& data);

private:
	StatusData& data;

	void Run() override;
};
