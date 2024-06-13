cmake_minimum_required(VERSION 3.20.5)

project(playground CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (BUILD_SHARED_LIBS OFF)
set(CMAKE_DEBUG_POSTFIX "")


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

#SDL Image Dep
set (SDL2_IMAGE_DIR ${PROJECT_SOURCE_DIR}/SDL2_Image)
set (SDL2_IMAGE_INCLUDE_DIRS ${SDL2_IMAGE_DIR}/include)
add_subdirectory(${SDL2_IMAGE_DIR} EXCLUDE_FROM_ALL)
find_library(SDL2image ${SDL2_IMAGE_DIR})

#Assimp
set (ASSIMP_DIR ${PROJECT_SOURCE_DIR}/Assimp)
set (ASSIMP_INCLUDE_DIRS ${ASSIMP_DIR}/include)
add_subdirectory(${ASSIMP_DIR} EXCLUDE_FROM_ALL)
find_library(Assimp ${ASSIMP_DIR})

# 0MQ Dep
set (ZeroMQ_DIR ${PROJECT_SOURCE_DIR}/ZeroMQ)
set (ZeroMQ_INCLUDE_DIRS ${ZeroMQ_DIR}/include)
add_subdirectory(${ZeroMQ_DIR} EXCLUDE_FROM_ALL)
find_library(ZeroMQ ${ZeroMQ_DIR})

# GLM Dep
set (GLM_DIR ${PROJECT_SOURCE_DIR}/glm/glm)

# IMGUI
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/imgui)

#####################
# SOURCES & HEADERS #
#####################

# Sources
file(GLOB_RECURSE PROJECT_SOURCE_FILES "src/*.cpp")
set(projectSources
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${PROJECT_SOURCE_FILES}
)

# Include dirs.
set(projectIncludeDirs ${projectIncludeDirs}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${ZeroMQ_INCLUDE_DIRS}
    ${GLM_DIR}
    ${ASSIMP_INCLUDE_DIRS}
)
include_directories(${projectIncludeDirs})


###########
# TARGETS #
###########

# Spirv Shaders (has to be run manually)
add_custom_target(
        compile_spirv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/color_meshes.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/color_meshes.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/color_meshes.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/color_meshes.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/color_meshes_gpu.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/color_meshes_gpu.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/color_meshes_gpu.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/color_meshes_gpu.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/vertex_meshes_gpu.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/vertex_meshes_gpu.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/cull-vertex.comp -o ${PROJECT_SOURCE_DIR}/assets/shaders/cull-vertex.comp.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/cull-indexed.comp -o ${PROJECT_SOURCE_DIR}/assets/shaders/cull-indexed.comp.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/normals.geom -o ${PROJECT_SOURCE_DIR}/assets/shaders/normals.geom.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/skybox.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/skybox.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/skybox.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/skybox.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/texture_meshes.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/texture_meshes.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/texture_meshes.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/texture_meshes.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/texture_meshes_gpu.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/texture_meshes_gpu.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/texture_meshes_gpu.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/texture_meshes_gpu.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/model_meshes.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/model_meshes.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/model_meshes.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/model_meshes.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/model_meshes_gpu.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/model_meshes_gpu.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/model_meshes_gpu.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/model_meshes_gpu.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/animated_model_meshes.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/animated_model_meshes.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/animated_model_meshes.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/animated_model_meshes.frag.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/animated_model_meshes_gpu.vert -o ${PROJECT_SOURCE_DIR}/assets/shaders/animated_model_meshes_gpu.vert.spv
        COMMAND glslc ${PROJECT_SOURCE_DIR}/src/shaders/animated_model_meshes_gpu.frag -o ${PROJECT_SOURCE_DIR}/assets/shaders/animated_model_meshes_gpu.frag.spv
)


# executable
add_executable(playground ${projectSources})
target_link_libraries(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    SDL2-static
    SDL2main
    SDL2_image
    libzmq-static
    assimp
)
