// #version 450

// layout(location = 0) out vec3 fragColor;

// vec2 positions[3] = vec2[](
//     vec2(0.0, -0.5),
//     vec2(0.5, 0.5),
//     vec2(-0.5, 0.5)
// );

// vec3 colors[3] = vec3[](
//     vec3(1.0, 0.0, 0.0),
//     vec3(0.0, 1.0, 0.0),
//     vec3(0.0, 0.0, 1.0)
// );

// void main() {
//     gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
//     fragColor = colors[gl_VertexIndex];
// }

#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoords;
layout(location = 2) out vec3 fragNormals;
layout(location = 3) out flat int fragTexIndex;

layout(buffer_reference, std140) buffer readonly UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(buffer_reference, scalar) buffer readonly SceneUniforms {
    mat4 view;
    mat4 proj;
    vec4 camera_position;
};

layout(buffer_reference, scalar) buffer ObjectsTable {
    mat4 models[];
};

layout(push_constant) uniform Constants {
    int texture_index;
    int model_matrix_id;
    SceneUniforms scene_ubo_address;
    ObjectsTable models_addr;
} push_const;


/*

layout(buffer_reference, scalar) buffer readonly SceneUniforms {
    mat4 view;
    mat4 proj;
    vec4 camera_pos;
};

const int max_point_lights = 1000;
layout(buffer_reference, scalar) buffer readonly PointLightsUbo {
    int num_lights;
    int pad_0; // 4
    int pad_1; // 8
    int pad_2; // 12
    point_light point_light_sources[max_point_lights];
};

// 3D Geometry transform matrix - access by index (ID)
layout(buffer_reference, scalar) uniform ObjectsTable {
    mat4 model[];
};

layout(push_constant) uniform Constants {
    SceneUniforms scene_data;   // access scene-wide uniforms
    ObjectsTable objects;       // access array of a geoemtry model matrix via "model_matrix_id"
    int model_matrix_idx;       // accessing by index into ObjectsTable
    int diffuse_idx;            // index to access diffuse texture
    int specular_idx;           // index to access specular index
    PointLightsUniform lights;  // Accessing point lights
} push_const;

*/


void main() {
    // UniformBufferObject ubo = push_const.global_ubo_address;
    SceneUniforms scene_ubo = push_const.scene_ubo_address;
    ObjectsTable objects = push_const.models_addr;

    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    vec4 world_position = objects.models[push_const.model_matrix_id] * vec4(inPosition, 1.0);
    gl_Position = scene_ubo.proj * scene_ubo.view * world_position;
    fragColor = inColor;
    fragTexCoords = inTexCoords;
    fragNormals = inNormals;
    fragTexIndex = push_const.texture_index;
}