cmake_minimum_required(VERSION 3.12)
project(mgit VERSION 1.0)

set(HEADERS
    
)

set(SOURCES
	src/main.cpp
)

add_executable(
    mgit
    "${HEADERS}"
    "${SOURCES}"
)

