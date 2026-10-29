#include <print>
#include <algorithm>
#include <cmath>
#include <span>
#include <variant>
#include <array>
#include <vector>
#include <queue>
#include <list>

// topological sort demo example


void normal_topological_sort() {
    std::vector<std::vector<int>> adj(6);
    std::vector<int> indegree(adj.size(), 0);
    std::queue<int> q;
    std::list<int> list;


    // add_edge(u: 0, v: 1);
    // add_edge(u: 1, v: 2);
    // add_edge(u: 2, v: 3);
    // add_edge(u: 4, v: 5);
    // add_edge(u: 5, v: 1);
    // add_edge(u: 5, v: 2);
    adj[0].push_back(1);
    adj[1].push_back(2);
    adj[2].push_back(3);
    adj[4].push_back(5);
    adj[5].push_back(1);
    adj[5].push_back(2);



    // Computing the indegree
    for(int i = 0; i < adj.size(); i++) {
        for(int next : adj[i]) {
            indegree[next]++;
        }
    }

    // Adding all nodes that have been set to indegree of 0
    // into the queue
    for(int i = 0; i < adj.size(); i++) {
        if(indegree[i] == 0) {
            q.push(i);
        }
    }

    // Kahns Algorithm (BFS)
    while(!q.empty()) {
        int top = q.front();
        q.pop();
        list.push_back(top);

        for(int next : adj[top]) {
            indegree[next]--;

            if(indegree[next] == 0) {
                q.push(next);
            }
        }
    }

    for(const auto& i : list) {
        std::print("{} ", i);
    }

    std::println();
}

/**
 * @brief Represents our renderpass
 * 
 * Each renderpass is going to have specific inputs/outputs for specify the dependency of a particular render pass.
 */
struct renderpass_node {
    std::string name="";
    std::vector<uint32_t> inputs; // depends on
    std::vector<uint32_t> outputs; // renderpass outputs to
    uint32_t indegree=0;
};

// Still only keeping track of the indices
void topological_sort_demo1() {
    std::vector<renderpass_node> nodes(3);

    // add_edge(u: 0, v: 1);
    // add_edge(u: 1, v: 2);
    // add_edge(u: 2, v: 3);
    // add_edge(u: 4, v: 5);
    // add_edge(u: 5, v: 1);
    // add_edge(u: 5, v: 2);
    renderpass_node gbuffer = {
        .name = "GBUffer",
        .inputs = {1}
    };

    renderpass_node lighting_pass = {
        .name = "Lighting",
        .inputs = {2}
    };

    renderpass_node shadow_pass = {
        .name = "Shadow Pass",
        .inputs = {3}
    };



    std::vector<uint32_t> execution_order;
    execution_order.reserve(nodes.size());

    std::queue<uint32_t> zero_indegree_queue;

    for(uint32_t i = 0; i < nodes.size(); i++) {
        // Sertting our indegrees
        nodes[i].indegree = nodes[i].inputs.size();

        // Checking if any starting nodes have been set to indegrees of 0
        if(nodes[i].indegree == 0) {
            zero_indegree_queue.push(i);
        }
    }

    while(!zero_indegree_queue.empty()) {
        uint32_t current = zero_indegree_queue.front();
        zero_indegree_queue.pop();

        execution_order.push_back(current);


        for(uint32_t successor : nodes[current].outputs) {
            nodes[successor].indegree--;

            if(nodes[successor].indegree == 0) {
                zero_indegree_queue.push(successor);
            }
        }
    }

    for(const auto& i : execution_order) {
        std::print("{} ", i);
    }

    std::println();
}

int main() {

    std::vector<renderpass_node> passes(4); // 0: GBuffer, 1: Lighting, 2: Shadow

    // GBuffer (root): feeds to lighting
    renderpass_node gbuffer = {
        .name = "GBUffer",
        .outputs = {1}, // lighting-pass depends on this; outputs to lighting
    };

    // Lighting: Depends on GBuffer and Shadows, feeds to UI
    renderpass_node lighting_pass = {
        .name = "Lighting",
        .inputs = {0}, // depends on GBuffer
        .outputs = {3}, // outputs to UI pass
    };

    // Shadows (root): Feeds into Lighting
    renderpass_node shadow_pass = {
        .name = "Shadow Pass",
        .outputs = {1}, // Outputs to Lighting
    };

    renderpass_node ui_pass = {
        .name = "UI Pass",
        .inputs = {1}, // 
    };



    /*
    
    
    GBuffer (0) --->
               | -> Lighting (2) --> UI (3)
    Shadow (0) --->
    
    
    
    */

    passes[0] = gbuffer;
    passes[1] = lighting_pass;
    passes[2] = shadow_pass;
    passes[3] = ui_pass;

    std::vector<renderpass_node> execution_order;
    execution_order.reserve(passes.size());

    std::queue<uint32_t> zero_indegree_queue;

    for(uint32_t i = 0; i < passes.size(); i++) {
        passes[i].indegree = passes[i].inputs.size();

        if(passes[i].indegree == 0) {
            zero_indegree_queue.push(i);
        }
    }

    // Iterate through all of the zero-indegrees (given dependencies count)
    while(!zero_indegree_queue.empty()) {
        // Current resource ID = current
        // Used to provide the order of renderpasses into a given execution order
        uint32_t current = zero_indegree_queue.front();
        zero_indegree_queue.pop();

        execution_order.push_back(passes[current]);

        // Analyze the given output dependencies that is used
        // to help decrease the indegrees back into the std::queue
        for(uint32_t successor : passes[current].outputs) {
            passes[successor].indegree--;

            if(passes[successor].indegree == 0) {
                zero_indegree_queue.push(successor);
            }
        }
    }

    for(const auto& renderpass : execution_order) {
        std::print("{} | ", renderpass.name);
    }

    std::println();

    return 0;
}