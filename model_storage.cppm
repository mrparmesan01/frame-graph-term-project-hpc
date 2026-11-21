module;

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <span>
#include <string_view>
#include <filesystem>
#include <unordered_map>
#include <print>

export module model_storage;
import vk;
import types;
import obj_model;
import uniform_types;

export {
    class model_storage {
    public:
        model_storage() = delete;
        model_storage(vk::device p_device, vk::physical_device p_physical) : m_device(p_device), m_physical(p_physical) {}

        ~model_storage() = default;


        void add_model(std::string_view p_name, const std::filesystem::path& p_path) {
            obj_model model(p_path, m_device, m_physical);
            if(!model.loaded()) {
                std::println("Path: {}  failed to load!", p_path.string());
            }

            m_models.emplace(p_name, model);
            uint32_t idx = m_model_index++;
            m_model_matrix_lookup.emplace(p_name, idx);
            m_model_matrices.emplace_back(1.f);
        }

        [[nodiscard]] bool contains(std::string_view p_name) {
            return m_models.contains(p_name);
        }

        std::optional<obj_model> get(std::string_view p_name) {
            return m_models.at(p_name);
        }

        void write_model(std::string_view p_name, const transform& p_transform) {
            glm::mat4 model(1.f);
            model = glm::translate(model, p_transform.position);
            model = glm::scale(model, p_transform.scale);

            if(!m_model_matrix_lookup.contains(p_name)) {
                return;
            }

            uint32_t idx = m_model_matrix_lookup[p_name];
            m_model_matrices[idx] = model;
        }


        uint32_t get_index(std::string_view p_name) {
            return m_model_matrix_lookup[p_name];
        }

        std::span<glm::mat4> model_matrices() {
            return m_model_matrices;
        }

        uint32_t vertices_size(std::string_view p_name) {
            return m_models[p_name].vertices_size();
        }

        uint32_t indices_size(std::string_view p_name) {
            return m_models[p_name].indices_size();
        }

        void destruct() {
            for(auto&[name, model] : m_models) {
                model.destruct();
            }
        }

    private:
        vk::device m_device;
        vk::physical_device m_physical;

        uint32_t m_model_index = 0;

        std::unordered_map<std::string_view, obj_model> m_models;
        std::unordered_map<std::string_view, uint32_t> m_model_matrix_lookup;
        std::vector<glm::mat4> m_model_matrices;
    };
}