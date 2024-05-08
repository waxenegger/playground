cmake_minimum_required(VERSION 3.20.5)

project(playground CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

set (SDL2_DIR ${PROJECT_SOURCE_DIR}/SDL2)
set (SDL2_INCLUDE_DIRS ${SDL2_DIR}/include)
add_subdirectory(${SDL2_DIR} EXCLUDE_FROM_ALL)
find_library(SDL2main ${SDL2_DIR})

set (ZeroMQ_DIR ${PROJECT_SOURCE_DIR}/ZeroMQ)
set (ZeroMQ_INCLUDE_DIRS ${ZeroMQ_DIR}/include)
add_subdirectory(${ZeroMQ_DIR} EXCLUDE_FROM_ALL)
find_library(ZeroMQ ${ZeroMQ_DIR})

# Sources
file(GLOB_RECURSE PROJECT_SOURCE_FILES "src/*.cpp")
set(projectSources
    ${PROJECT_SOURCE_FILES}
)

# Include dirs.
set(projectIncludeDirs ${projectIncludeDirs}
    ${Vulkan_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${ZeroMQ_INCLUDE_DIRS}
)
include_directories(${projectIncludeDirs})

add_executable(playground ${projectSources})
target_link_libraries(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    SDL2-static
    SDL2main
    libzmq-static
)
