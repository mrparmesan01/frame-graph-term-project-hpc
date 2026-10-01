cmake_minimum_required(VERSION 4.0)
project(buffer-device-address CXX)

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

    LINK_PACKAGES
    stb::stb
    vulkan-cpp
    tinyobjloader
    Vulkan::Vulkan
)

target_sources(${PROJECT_NAME} PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES
    vertex.cppm
    image.cppm
    obj_model.cppm
)