module;

#include <cstdint>
#include <span>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

export module uniform_types;


export {

    struct global_uniform {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // TEMP DEF: Will remove to add lighting eventually
    struct push_constant_data {
        uint32_t texture_index = 0;
        uint32_t model_matrix_id = 0;
        uint64_t scene_ubo_addr = 0;
        uint64_t model_matrices_addr = 0;
    };

    struct scene_ubo {
        glm::mat4 view = glm::mat4(1.f);
        glm::mat4 proj = glm::mat4(1.f);
        glm::vec4 camera_pos = glm::vec4(1.f);
    };

    struct matrix_model_ubo {
        std::span<glm::mat4> model_matrices;
    };

    // 0 = defaults to white texture
    struct gpu_material {
        uint32_t diffuse_idx = 0;
        uint32_t specular_idx = 0;
    };


    // TODO: Will use this
    struct push_const_data {
        uint64_t scene_address = 0;
        uint64_t model_mat_arr_addr = 0;
        uint64_t model_idx = 0;
        uint64_t diffuse_idx = 0;
        uint64_t specular_idx = 0;
        uint64_t point_light_arr_addr = 0;
    };

    //! @brief converts vec4 to quaterion
    glm::quat to_quat(const glm::vec4& p_values) {
        return glm::quat({
          p_values.w,
          p_values.x,
          p_values.y,
          p_values.z,
        });
    }

    //! @brief converts vec3 to quaterion
    glm::highp_vec4 from_quat(const glm::vec3& p_values) {
        // converts glm::vec3 rotation to a quaternion returning the
        // quaternion-converted values to glm::highp_vec4
        auto quaternion = glm::quat(p_values);
        return glm::vec4(
          { quaternion.x, quaternion.y, quaternion.z, quaternion.w });
    }

    // scene object ubo
    struct transform {
        glm::highp_vec3 position;
        glm::highp_vec3 scale;
        glm::highp_vec3 rotation;
        glm::highp_vec4 quaternion{ 0.f, 0, 0, 1 };

        void set_rotation(const glm::highp_vec3& p_value) {
            rotation = p_value;
            quaternion = from_quat(rotation);
        }
    };

    // Uniforms for point lights
    struct point_light_ubo {
        glm::vec4 position; // provided by transform struct btw
        glm::vec4 color = glm::vec4(1.f);
        float attenuation = 1.f;
        float constant = 1.f;
        float linear = 1.f;
        float quadratic = 1.f;

        glm::vec3 ambient{1.f};
        glm::vec3 diffuse{1.f};
        glm::vec3 specular{1.f};
    };


    // Scene/Camera Components

    struct mesh_source {
        std::string model_path="";
        std::string diffuse_path="";
        std::string specular_path="";
    };

    struct perspective_camera {
        glm::vec2 plane{0.f};
        bool is_active=false;
        float field_of_view = glm::radians(45.f);
    };


    struct editor {};

    struct projection_view {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
    };

};