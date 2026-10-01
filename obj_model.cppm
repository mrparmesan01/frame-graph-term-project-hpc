
module;

#include <vector>
#include <string>
#include <unordered_map>
#include <print>
#include <filesystem>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLFW_INCLUDE_VULKAN
#if _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#endif

export module obj_model;

import vk;

import vertex;


export {

// Part of this demo for loading a 3D .obj model
class obj_model {
public:
    obj_model() = default;
    obj_model(const std::filesystem::path& p_filename,
              const VkDevice& p_device,
              const vk::physical_device& p_physical) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        //! @note If we return the constructor then we can check if the mesh
        //! loaded successfully
        //! @note We also receive hints if the loading is successful!
        //! @note Return default constructor automatically returns false means
        //! that mesh will return the boolean as false because it wasnt
        //! successful
        if (!tinyobj::LoadObj(&attrib,
                              &shapes,
                              &materials,
                              &warn,
                              &err,
                              p_filename.string().c_str())) {
            std::println("Could not load model from path {}",
                         p_filename.string());
            m_is_loaded = false;
            return;
        }

        std::vector<vk::vertex_input> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<vk::vertex_input, uint32_t> unique_vertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vk::vertex_input vertex{};

                // vertices.push_back(vertex);
                if (!unique_vertices.contains(vertex)) {
                    unique_vertices[vertex] =
                      static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                if (index.vertex_index >= 0) {
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2]
                    };
                }

                if (index.normal_index >= 0) {
                    vertex.normals = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                if (!unique_vertices.contains(vertex)) {
                    unique_vertices[vertex] =
                      static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(unique_vertices[vertex]);
            }
        }

        m_has_indices = (indices.size() > 0) ? true : false;

        if (m_has_indices) {
            m_indices_size = indices.size();
        }
        m_indices_size = vertices.size();
        m_indices_size = indices.size();

        vk::buffer_parameters vertex_params = {
            .memory_mask = p_physical.memory_properties(
              vk::memory_property::device_local_bit |
              vk::memory_property::host_visible_bit),
            .usage = vk::buffer_usage::transfer_dst_bit |
                     vk::buffer_usage::vertex_buffer_bit,
        };

        vk::buffer_parameters index_params = {
            .memory_mask = p_physical.memory_properties(
              vk::memory_property::host_visible_bit |
              vk::memory_property::host_cached_bit),
            .usage = vk::buffer_usage::index_buffer_bit,
        };

        m_vertex_buffer = vk::vertex_buffer(p_device, vertices, vertex_params);
        m_index_buffer = vk::index_buffer(p_device, indices, index_params);
        m_is_loaded = true;
    }

    [[nodiscard]] bool loaded() const { return m_is_loaded; }

    [[nodiscard]] VkBuffer vertex_handle() const { return m_vertex_buffer; }

    [[nodiscard]] VkBuffer index_handle() const { return m_index_buffer; }

    [[nodiscard]] bool has_indices() const { return m_has_indices; }

    [[nodiscard]] uint32_t indices_size() const { return m_indices_size; }

    void draw(const VkCommandBuffer& p_command) {
        if (m_has_indices) {
            vkCmdDrawIndexed(p_command, m_indices_size, 1, 0, 0, 0);
        }
        else {
            vkCmdDraw(p_command, m_indices_size, 1, 0, 0);
        }
    }

    void destruct() {
        m_vertex_buffer.destruct();
        m_index_buffer.destruct();
    }

private:
    bool m_is_loaded = false;
    bool m_has_indices = false;
    uint32_t m_indices_size = 0;
    vk::vertex_buffer m_vertex_buffer{};
    vk::index_buffer m_index_buffer{};
};
};
