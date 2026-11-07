module;

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <unordered_map>
#include <print>
#include <algorithm>
#include <cmath>
#include <span>
#include <variant>
#include <array>
#include <vector>
#include <string_view>
#include <queue>
#include <list>
#include <expected>
#include <ranges>
#include <variant>

export module frame_graph;

import vk;
import types;

export {
/**
* @brief Stores GPU-transient resources such as images and buffers
* Performs resource look-ups via resource_id
*/
class resource_storage {
public:
    resource_storage() = default;
    
    resource_id add_image(std::string_view p_name, uint32_t p_width, uint32_t p_height, vk::format p_format) {
        // resource_id id = m_current_resource_id++;
        // m_resources.push_back(resource_description{
        //     .name = p_name.data(),
        //     .id = id,
        //     .resource = image_resource{ .width = p_width, .height = p_height },
        // });
        // m_resource_ids[p_name] = id;
        // return id;
        return {};
    }

    resource_id add_buffer(std::string_view p_name, uint64_t p_size_bytes) {
        // resource_id id = m_current_resource_id++;
        // m_resources.push_back(resource_description{
        //     .name = p_name.data(),
        //     .id = id,
        //     .resource = buffer_resource{ .size_bytes = p_size_bytes },
        // });

        // m_resource_ids[p_name] = id;

        // return id;
        return {};
    }

private:
};


class render_graph {
public:

    pass_id emplace_pass(std::string_view p_name, std::span<const resource_id> p_reads, std::span<const resource_id> p_writes, std::function<void(vk::command_buffer)> p_callback) {
        return {};
    }

    void compile() {}

    void execute(vk::command_buffer p_command) {}

    void print_dependencies() {
        std::println("Printing render_graph dependencies!");
    }

private:
};

};