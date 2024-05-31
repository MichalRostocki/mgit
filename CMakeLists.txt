cmake_minimum_required(VERSION 3.12)
project(mgit VERSION 1.0)



set(HEADERS
    src/json.hpp
)

set(SOURCES
	src/main.cpp
)

add_executable(
    mgit
    "${HEADERS}"
    "${SOURCES}"
)
set_property(TARGET mgit PROPERTY CXX_STANDARD 23)
