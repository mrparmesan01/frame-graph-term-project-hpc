module;

#include <cstdint>
#include <glm/glm.hpp>

export module uniform_types;


export {

    struct scene_ubo {
        glm::mat4 view = glm::mat4(1.f);
        glm::mat4 proj = glm::mat4(1.f);
        glm::vec4 camera_pos = glm::vec4(1.f);
    };

    // scene object ubo
    struct transform {
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
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

};