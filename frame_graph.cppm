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
#include <unordered_set>

export module frame_graph;

import vk;
import types;
import resource_storage;

inline vk::v2::pipeline_stage_flags operator|(vk::v2::pipeline_stage_flags p_lhs,
                                 vk::v2::pipeline_stage_flags p_rhs) {
    // Lets us truncate the underlying type of the enum (class) to allow
    // it to be bitwise OR'd
    using T = std::underlying_type_t<vk::v2::pipeline_stage_flags>;
    return static_cast<vk::v2::pipeline_stage_flags>(static_cast<T>(p_lhs) |
                                        static_cast<T>(p_rhs));
}

export {

// Enum values to evaluate the use for description for that particular resource
enum class resource_usage {
    color_attachment_write,
    depth_stencil_write,
    depth_stencil_read,
    compute_shader_write,
    compure_shader_read,
    fragment_shader_sample,
    transfer_src,
    transfer_dst,
    present
};

// Parameters to evaluate how this particular resource is going to be written
struct resource_write {
    resource_id id;
    resource_usage usage;
};

struct resource_barrier {
    enum vk::image_layout layout = vk::image_layout::undefined;
    vk::v2::pipeline_stage_flags stage_flags;
    vk::v2::access_flags access;
    bool is_write = false;
};

struct renderpass_node {
    std::string name="";
    std::vector<uint32_t> writes; // renderpass writes to (->)
    std::vector<uint32_t> reads; // renderpass stages to read from (<-)
    uint32_t indegree=0;
};

class frame_graph {
public:


    pass_id emplace_pass(std::string_view p_name, std::span<resource_id> p_reads, std::span<resource_id> p_writes, std::span<const resource_write> p_write_description, std::function<void(vk::command_buffer, resource_storage&)> p_callback) {
        pass_id id = m_current_pass_id++;
        m_name_to_id_table[p_name] = id;
        pass_description desc = {p_name, id, {p_reads.data(), p_reads.size()}, {p_writes.data(), p_writes.size()}};

        m_passes.emplace_back(desc);
        m_execute_commands.emplace(id, p_callback);
        return id;
    }



    void compile() {
        std::println("Analyzing Dependencies!");

        m_execution_order.reserve(m_passes.size());
        std::vector<std::unordered_set<pass_id>> adjacency_list(m_passes.size());
        std::unordered_map<resource_id, pass_id> resource_producer;
        std::vector<uint32_t> indegrees(m_passes.size());
        std::queue<uint32_t> zero_indegrees;

        for(const auto& pass : m_passes) {
            for(auto w : pass.writes) {
                resource_producer[w] = pass.id;
            }
        }

        for(const auto& pass : m_passes) {
            for(auto r : pass.reads) {
                if(resource_producer.contains(r)) {
                    pass_id producer = resource_producer[r];
                    if(producer != pass.id) {
                        adjacency_list[producer].insert(pass.id);
                    }
                }
            }
        }

        for(size_t i = 0; i < m_passes.size(); i++) {
            for(const auto& succ : adjacency_list[i]) {
                indegrees[succ]++;
            }
        }

        for(size_t i = 0; i < m_passes.size(); i++) {
            if(indegrees[i] == 0) {
                zero_indegrees.push(i);
            }
        }

        while(!zero_indegrees.empty()) {
            pass_id current = zero_indegrees.front();
            zero_indegrees.pop();

            for(const auto& pass : m_passes) {
                if(pass.id == current) {
                    m_execution_order.push_back(pass);
                    break;
                }
            }

            for(pass_id successor : adjacency_list[current]) {
                if(--indegrees[successor] == 0) {
                    zero_indegrees.push(successor);
                }
            }
        }

        if(m_execution_order.size() != m_passes.size()) {
            std::println("Render graph cycle detected!");
        }
    }

    void dependencies_synchronization(vk::command_buffer p_command, resource_storage& p_storage, const pass_description& p_pass) {
        for(auto write : p_pass.writes) {
            auto image_opt = p_storage.get_image(p_pass.name);

            if(image_opt.has_value()) {
                vk::v2::image_memory_barrier write_barrier = {
                    .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe,
                    .src_access_mask = vk::v2::access_flags::memory_read_bit,
                    .dst_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output,
                    .dst_access_mask = vk::v2::access_flags::color_attachment_write_bit,
                    .old_layout      = vk::image_layout::undefined,
                    .new_layout      = vk::image_layout::color_optimal,
                    .subresource_range = {
                        .aspect_mask = vk::image_aspect_flags::color_bit,
                        .level_count = 1,
                        .layer_count = 1,
                    },
                };

                auto img = image_opt.value();

                img.pipeline_barriers(p_command, std::views::single(write_barrier));
            }
        }

        for(auto read : p_pass.reads) {
            auto image_opt = p_storage.get_image(p_pass.name);

            if(image_opt.has_value()) {
                auto old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
                auto new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                auto img = image_opt.value();

                vk::v2::image_memory_barrier read_barrier = {
                    .src_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output,
                    .src_access_mask = vk::v2::access_flags::color_attachment_write_bit,
                    .dst_stage_mask  = vk::v2::pipeline_stage_flags::transient_bit,
                    .dst_access_mask = vk::v2::access_flags::transfer_read_bit,
                    .old_layout      = vk::image_layout::color_optimal,
                    .new_layout      = vk::image_layout::transfer_src_optimal,
                    .subresource_range = {
                        .aspect_mask = vk::image_aspect_flags::color_bit,
                        .level_count = 1,
                        .layer_count = 1,
                    },
                };
                img.pipeline_barriers(p_command, std::views::single(read_barrier));

            }
        }
    }

    void execute(vk::command_buffer p_command, resource_storage& p_storage) {
        for(const auto& pass : m_execution_order) {

            dependencies_synchronization(p_command, p_storage, pass);

            if(m_execute_commands.contains(pass.id)) {
                m_execute_commands.at(pass.id)(p_command, p_storage);
            }
        }
    }

    void print_dependencies() {
        std::println("Printing frame_graph dependencies!");

        for(const auto& renderpass : m_execution_order) {
            std::print("{} | ", renderpass.name);
        }


        std::println();
    }

private:
    // Perform automated barriers calls and batching
    void synchronize_barriers() {
    }


private:
    [[nodiscard]] constexpr resource_barrier make_barrier(resource_usage p_usage) const noexcept {
        using pipeline_stage_flags = vk::v2::pipeline_stage_flags;
        using access_flags = vk::v2::access_flags;

        switch(p_usage) {
            // Color attachments are written via color output
            case resource_usage::color_attachment_write:
                return {
                    .layout = vk::image_layout::color_optimal,
                    .stage_flags = pipeline_stage_flags::color_attachment_output,
                    .access = access_flags::color_attachment_write_bit,
                    .is_write = true,
                };
            case resource_usage::depth_stencil_write:
                // Depth/stencil writes can occur in earlier or later stages
                return {
                    .layout = vk::image_layout::depth_stencil_optimal,
                    .stage_flags = pipeline_stage_flags::early_fragment_shader_bit | pipeline_stage_flags::late_fragment_tests_bit,
                    .access = access_flags::depth_stencil_attachment_write_bit,
                    .is_write = true,
                };
            case resource_usage::depth_stencil_read:
                // Depth/stencil reads only happen in the earlier stages
                return {
                    .layout = vk::image_layout::depth_stencil_read_only_optimal,
                    .stage_flags = pipeline_stage_flags::early_fragment_shader_bit,
                    .access = access_flags::depth_stencil_attachment_read_bit,
                    .is_write = false,
                };
            case resource_usage::fragment_shader_sample:
                // Shader samplers occur in the fragment stages
                return {
                    .layout= vk::image_layout::shader_read_only_optimal,
                    .stage_flags = pipeline_stage_flags::transient_bit, // TODO: Change pipeline_stage_flags::blit_bit
                    .access = access_flags::shader_read_bit,
                    .is_write = false,
                };
            case resource_usage::transfer_src:
                // Blit/copy source reads
                return {
                    .layout = vk::image_layout::transfer_src_optimal,
                    .stage_flags = pipeline_stage_flags::transient_bit, // TODO: Change pipeline_stage_flags::blit_bit
                    .access = access_flags::transfer_read_bit,
                    .is_write = false,
                };
            case resource_usage::transfer_dst:
                // Blit/copy destination writes
                return {
                    .layout = vk::image_layout::transfer_dst_optimal,
                    .stage_flags = pipeline_stage_flags::transient_bit, // TODO: Change pipeline_stage_flags::blit_bit
                    // .access = ..., // Undefined
                    .is_write = false,
                };
            case resource_stage::present:
                // Final layout required before presenting to the swapchain
                return {
                    .layout = vk::image_layout::present_src_khr,
                    .stage_flags = pipeline_stage_flags::bottom_of_pipe,
                    // .access = ..., // Undefined
                    .is_write = false,
                };
            default:
                // Fallback to undefined/top-of-pipe flag
                return {};
        }
    }

private:
    pass_id m_current_pass_id=0;
    std::unordered_map<std::string_view, pass_id> m_name_to_id_table;
    std::unordered_map<pass_id, resource_barrier> m_barrier_descriptions;
    std::unordered_map<pass_id, std::function<void(vk::command_buffer, resource_storage&)>> m_execute_commands;
    std::vector<pass_description> m_passes;
    std::vector<pass_description> m_execution_order;
};

};