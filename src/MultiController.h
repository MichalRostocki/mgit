#pragma once
#include <list>

#include "RepositoryController.h"

class MultiController
{
public:
    bool LoadConfig(std::ostream& error_stream);

    void DisplayStatus(std::ostream& output_stream) const;

    MultiController() = default;
    ~MultiController();

    MultiController(const MultiController& other) = delete;
    MultiController(MultiController&& other) noexcept = delete;
    MultiController& operator=(const MultiController& other) = delete;
    MultiController& operator=(MultiController&& other) noexcept = delete;

private:
    std::list<RepositoryController> controllers;
};

