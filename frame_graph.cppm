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

export {

struct renderpass_node {
    std::string name="";
    std::vector<uint32_t> writes; // renderpass writes to (->)
    std::vector<uint32_t> reads; // renderpass stages to read from (<-)
    uint32_t indegree=0;
};

class frame_graph {
public:


    pass_id emplace_pass(std::string_view p_name, std::span<resource_id> p_reads, std::span<resource_id> p_writes, std::function<void(vk::command_buffer, resource_storage&)> p_callback) {
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

    void execute(vk::command_buffer p_command, resource_storage& p_storage) {
        for(const auto& pass : m_execution_order) {
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
    pass_id m_current_pass_id=0;
    std::unordered_map<std::string_view, pass_id> m_name_to_id_table;
    std::unordered_map<pass_id, std::function<void(vk::command_buffer, resource_storage&)>> m_execute_commands;
    std::vector<pass_description> m_passes;
    std::vector<pass_description> m_execution_order;
};

};