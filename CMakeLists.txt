cmake_minimum_required(VERSION 3.12)
project(mgit VERSION 1.0)

set(LIBGIT2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/../libgit2/include/" CACHE PATH "Path to libgit2 include files"  )
set(LIBGIT2_LIB_DIRS "${CMAKE_SOURCE_DIR}/../libgit2/bin/" CACHE PATH "Path to built libgit2 lib files"  )

set(HEADERS
    src/Config.h
    src/GitLibLock.h
    src/json.hpp
    src/MultiController.h
    src/OrderedMap.h
    src/RepoOrchestrator.h

    src/Data/Data.h

    src/Displays/Display.h
    src/Displays/PipelineDisplay.h
    src/Displays/StatusDisplay.h
    
    src/Tasks/CheckoutTask.h
    src/Tasks/CleanupTask.h
    src/Tasks/CommandTask.h
    src/Tasks/PullPrepareTask.h
    src/Tasks/PullTask.h
    src/Tasks/StatusTask.h
    src/Tasks/Task.h
)

set(SOURCES
    src/Config.cpp
    src/GitLibLock.cpp
    src/main.cpp
    src/MultiController.cpp
    src/RepoOrchestrator.cpp

    src/Data/Data.cpp

    src/Displays/Display.cpp
    src/Displays/PipelineDisplay.cpp
    src/Displays/StatusDisplay.cpp
    
    src/Tasks/CheckoutTask.cpp
    src/Tasks/CleanupTask.cpp
    src/Tasks/CommandTask.cpp
    src/Tasks/PullPrepareTask.cpp
    src/Tasks/PullTask.cpp
    src/Tasks/StatusTask.cpp
    src/Tasks/Task.cpp
)

add_executable(
    mgit
    "${HEADERS}"
    "${SOURCES}"
)

set_property(TARGET mgit PROPERTY CXX_STANDARD 23)

target_include_directories(mgit PUBLIC ${LIBGIT2_INCLUDE_DIRS})
target_link_libraries(mgit ${LIBGIT2_LIB_DIRS}/git2.lib WinHttp rpcrt4 crypt32)
target_compile_options(mgit PRIVATE "/MP")
target_include_directories(mgit PRIVATE src)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Headers" FILES ${HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${SOURCES})

install(TARGETS mgit DESTINATION bin)
