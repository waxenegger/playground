cmake_minimum_required(VERSION 3.20.5)

project(playground CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


################
# DEPENDENCIES #
################

# Vulkan Dep
find_package(Vulkan REQUIRED)

# SDL2 Dep
set (SDL2_DIR ${PROJECT_SOURCE_DIR}/SDL2)
set (SDL2_INCLUDE_DIRS ${SDL2_DIR}/include)
add_subdirectory(${SDL2_DIR} EXCLUDE_FROM_ALL)
find_library(SDL2main ${SDL2_DIR})

# 0MQ Dep
set (ZeroMQ_DIR ${PROJECT_SOURCE_DIR}/ZeroMQ)
set (ZeroMQ_INCLUDE_DIRS ${ZeroMQ_DIR}/include)
add_subdirectory(${ZeroMQ_DIR} EXCLUDE_FROM_ALL)
find_library(ZeroMQ ${ZeroMQ_DIR})

# GLM Dep
set (GLM_DIR ${PROJECT_SOURCE_DIR}/glm/glm)


#####################
# SOURCES & HEADERS #
#####################

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
    ${GLM_DIR}
)
include_directories(${projectIncludeDirs})


###########
# TARGETS #
###########

# Spirv Shaders (has to be run manually)
add_custom_target(
        compile_spirv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/test.vert -o ${PROJECT_SOURCE_DIR}/shaders/test.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/test.frag -o ${PROJECT_SOURCE_DIR}/shaders/test.frag.spv
)


# executable
add_executable(playground ${projectSources})
target_link_libraries(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    SDL2-static
    SDL2main
    libzmq-static
)
