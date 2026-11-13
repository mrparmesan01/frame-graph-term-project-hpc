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

struct renderpass_node {
    std::string name="";
    std::vector<uint32_t> writes; // renderpass writes to (->)
    std::vector<uint32_t> reads; // renderpass stages to read from (<-)
    uint32_t indegree=0;
};

class frame_graph {
public:

    // pass_id emplace_pass(std::string_view p_name, std::span<const resource_id> p_reads, std::span<const resource_id> p_writes, std::function<void(vk::command_buffer)> p_callback) {
    //     return {};
    // }

    void emplace_pass(std::string_view p_name, std::span<const uint32_t> p_writes, std::span<const uint32_t> p_reads) {
        renderpass_node pass_node = {
            .name = {p_name.data(), p_name.size()},
            .writes = std::vector<uint32_t>(p_writes.begin(), p_writes.end()),
            .reads = std::vector<uint32_t>(p_reads.begin(), p_reads.end()),
        };

        m_passes.emplace_back(pass_node);
    }



    void compile() {
        std::println("Analyzing Dependencies!");
        m_execution_order.reserve(m_passes.size());

        std::queue<uint32_t> zero_indegree_queue;

        for(uint32_t i = 0; i < m_passes.size(); i++) {
            m_passes[i].indegree = m_passes[i].writes.size();

            if(m_passes[i].indegree == 0) {
                zero_indegree_queue.push(i);
            }
        }

        while(!zero_indegree_queue.empty()) {
            uint32_t current = zero_indegree_queue.front();
            zero_indegree_queue.pop();

            m_execution_order.push_back(m_passes[current]);

            for(uint32_t successor : m_passes[current].reads) {
                m_passes[successor].indegree--;

                if(m_passes[successor].indegree == 0) {
                    zero_indegree_queue.push(successor);
                }
            }
        }
    }

    void execute(vk::command_buffer p_command) {}

    void print_dependencies() {
        std::println("Printing frame_graph dependencies!");

        for(const auto& renderpass : m_execution_order) {
            std::print("{} | ", renderpass.name);
        }

        std::println();
    }

private:
    std::vector<renderpass_node> m_passes{};
    std::vector<renderpass_node> m_execution_order;
};

};