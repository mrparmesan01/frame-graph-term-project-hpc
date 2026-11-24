cmake_minimum_required(VERSION 4.0)
project(frame-graph CXX)

build_application(
    SOURCES
    application.cpp

    PACKAGES
    vulkan-cpp
    Vulkan
    glfw3
    glm
    stb
    tinyobjloader
    flecs

    LINK_PACKAGES
    stb::stb
    vulkan-cpp
    tinyobjloader
    flecs::flecs_static
    Vulkan::Vulkan
)

target_sources(${PROJECT_NAME} PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES
    resource_storage.cppm
    vk_utilities.cppm
    vertex.cppm
    image.cppm
    obj_model.cppm
    types.cppm
    uniform_types.cppm
    frame_graph.cppm
    model_storage.cppm
    environment_map.cppm
)

# generate_compile_commands()