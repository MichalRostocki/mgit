#pragma once
#include "Config.h"

class Task;

class MultiController
{
public:
    bool LoadConfig(std::ostream& error_stream);

    void DisplayStatus(std::ostream& output_stream) const;
private:
    Config config;

    void RunTask(Task* task, std::ostream& output_stream) const;
};

