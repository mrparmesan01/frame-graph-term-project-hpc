#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

#include <print>
#include <array>
#include <ranges>
#include <expected>
#include <flecs.h>

import vk;
import vk_utilities;
import types;
import uniform_types;
import resource_storage;
import frame_graph;
import obj_model;
import model_storage;
import image;
import environment_map;
import math;
import events;

int
main() {

    if (!glfwInit()) {
        std::println("GLFW Error: glfwInit could not be initialized!\n");
        return -1;
    }

    if (!glfwVulkanSupported()) {
        std::println("GLFW Error: Vulkan is not supported!");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    int width = 800;
    int height = 600;
    GLFWwindow* window =
      glfwCreateWindow(width, height, "Render Graph", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    std::array<const char*, 1> validation_layers = {
        "VK_LAYER_KHRONOS_validation",
    };

    auto global_extensions = get_instance_extensions();

    vk::debug_message_utility debug_callback_info = {
        .severity =
          vk::message::verbose | vk::message::warning | vk::message::error,
        .message_type =
          vk::debug::general | vk::debug::validation | vk::debug::performance,
        .callback = debug_callback
    };

    vk::application_params config = {
        .name = "vulkan instance",
        .version = vk::api_version::vk_1_3, // specify to using vulkan 1.3
        .validations = validation_layers,
        .extensions = global_extensions
    };

    // 1. Setting up vk instance
    vk::instance api_instance(config, debug_callback_info);

    if (api_instance.alive()) {
        std::println("vk::instance is initialized!!");
    }

#if __APPLE__
    std::expected<vk::physical_device, VkResult> res =
      api_instance.enumerate_physical_device(vk::physical_gpu::integrated);
#else
    std::expected<vk::physical_device, VKResult> res =
      api_instance.enumerate_physical_device(vk::physical_gpu::discrete);
#endif

    vk::physical_device physical_device = res.value();

    std::array<float, 1> priorities = { 0.f };

#if defined(__APPLE__)
    std::array<const char*, 2> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
    };
#else
    std::array<const char*, 1> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
#endif

    std::array<vk::format, 3> format_support = {
        vk::format::d32_sfloat,
        vk::format::d32_sfloat_s8_uint,
        vk::format::d24_unorm_s8_uint
    };

    // We provide a selection of format support that we want to check is
    // supported on current hardware device.
    VkFormat depth_format =
      physical_device.request_depth_format(format_support);

    vk::device_features device_features{
        vk::dynamic_rendering_feature{ {
          .dynamicRendering = true,
        } },
        vk::descriptor_indexing_feature{ {
          .shaderSampledImageArrayNonUniformIndexing = true,
          .descriptorBindingSampledImageUpdateAfterBind = true,
          .descriptorBindingPartiallyBound = true,
          .descriptorBindingVariableDescriptorCount = true,
          .runtimeDescriptorArray = true,
        } },
        vk::buffer_device_address{ {
          .bufferDeviceAddress = true,
        } },
        vk::synchronization2{ {
          .synchronization2 = true,
        } },
    };

    vk::device_params logical_device_params = {
        .features = device_features.data(),
        .queue_priorities = priorities,
        .extensions = extensions,
        .queue_family_index = 0,
    };

    vk::device logical_device(physical_device, logical_device_params);

    vk::surface window_surface(api_instance, window);

    vk::surface_params surface_properties =
      physical_device.request_surface(window_surface);

    vk::swapchain_params enumerate_swapchain_settings = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .present_index = 0,
    };

    // (image_usage::transfer_src_bit | image_usage::transfer_dst_bit) required
    // for blitting/copying an image to swapchain_image
    vk::swapchain main_swapchain(logical_device,
                                 window_surface,
                                 enumerate_swapchain_settings,
                                 surface_properties,
                                 vk::image_usage::color_attachment_bit |
                                   vk::image_usage::transfer_src_bit |
                                   vk::image_usage::transfer_dst_bit);

    // querying presentable images
    std::span<const VkImage> images = main_swapchain.get_images();
    uint32_t image_count = static_cast<uint32_t>(images.size());

    // Creating Images
    std::vector<vk::sample_image> swapchain_images(image_count);
    std::vector<vk::sample_image> swapchain_depth_images(image_count);

    VkExtent2D swapchain_extent = surface_properties.capabilities.currentExtent;

    // Setting up the images
    uint32_t layer_count = 1;
    uint32_t mip_levels = 1;
    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
        vk::image_params swapchain_image_config = {
            .extent = { .width = swapchain_extent.width,
                        .height = swapchain_extent.height },
            .format = surface_properties.format.format,
            .memory_mask = physical_device.memory_properties(
              vk::memory_property::device_local_bit),
            .aspect = vk::image_aspect_flags::color_bit,
            .usage = (vk::image_usage::color_attachment_bit |
                      vk::image_usage::transfer_dst_bit),
            .mip_levels = 1,
            .layer_count = 1,
        };

        swapchain_images[i] =
          vk::sample_image(logical_device, images[i], swapchain_image_config);

        vk::image_params image_config = {
            .extent = {
                .width = swapchain_extent.width,
                .height = swapchain_extent.height,
            },
            .format = depth_format,
            .memory_mask = physical_device.memory_properties(
              vk::memory_property::device_local_bit),
            .aspect = vk::image_aspect_flags::depth_bit,
            .usage = vk::image_usage::depth_stencil_bit,
            .mip_levels = 1,
            .layer_count = 1,
        };
        swapchain_depth_images[i] =
          vk::sample_image(logical_device, image_config);
    }

    // setting up command buffers
    std::vector<vk::command_buffer> swapchain_command_buffers(image_count);
    for (size_t i = 0; i < swapchain_command_buffers.size(); i++) {
        vk::command_params settings = {
            .levels = vk::command_levels::primary,
            .queue_index = 0,
            .flags = vk::command_pool_flags::reset,
        };

        swapchain_command_buffers[i] =
          vk::command_buffer(logical_device, settings);
    }

    // setting up presentation queue to display commands to the screen
    vk::queue_params enumerate_present_queue{
        .family = 0,
        .index = 0,
    };
    vk::device_present_queue presentation_queue(
      logical_device, main_swapchain, enumerate_present_queue);

    // gets set with the renderpass
    std::array<float, 4> color = { 0.f, 0.5f, 0.5f, 1.f };

    // Loading graphics pipeline
    std::array<vk::shader_source, 2> shader_sources = {
        vk::shader_source{
          .filename = "shaders/test.vert.spv",
          .stage = vk::shader_stage::vertex,
        },
        vk::shader_source{
          .filename = "shaders/test.frag.spv",
          .stage = vk::shader_stage::fragment,
        },
    };

    // To render triangle, we do not need to set any vertex attributes
    vk::shader_resource_info shader_info = {
        .sources = shader_sources,
        .vertex_attributes = {} // this is to explicitly set to none, but also
                                // dont need to set this at all regardless
    };
    vk::shader_resource geometry_resource(logical_device, shader_info);

    // Setting up our vertex attributes
    std::array<vk::vertex_attribute_entry, 4> attribute_entries = {
        vk::vertex_attribute_entry{
          .location = 0,
          .format = vk::format::rgb32_sfloat,
          .stride = offsetof(vk::vertex_input, position),
        },
        vk::vertex_attribute_entry{
          .location = 1,
          .format = vk::format::rgb32_sfloat,
          .stride = offsetof(vk::vertex_input, color),
        },
        vk::vertex_attribute_entry{
          .location = 2,
          .format = vk::format::rg32_sfloat,
          .stride = offsetof(vk::vertex_input, uv),
        },
        vk::vertex_attribute_entry{
          .location = 3,
          .format = vk::format::rgb32_sfloat,
          .stride = offsetof(vk::vertex_input, normals),
        }
    };

    vk::vertex_attribute attributes = {
        .binding = 0,
        .entries = attribute_entries,
        .stride = sizeof(vk::vertex_input),
        .input_rate = vk::input_rate::vertex,
    };
    geometry_resource.vertex_attributes(std::views::single(attributes));

    // Initializing Descriptor Set 1
    uint32_t max_descriptors = 1000;
    vk::descriptor_entry descriptr_entry = {
        .type = vk::descriptor_type::combined_image_sampler,
        .binding_point = {
            .binding = 1,
            .stage = vk::shader_stage::fragment,
        },
        .descriptor_count = max_descriptors,
        .flags = vk::descriptor_bind_flags::partially_bound_bit | vk::descriptor_bind_flags::variable_descriptor_count_bit | vk::descriptor_bind_flags::update_after_bind,
    };

    vk::descriptor_layout set1_layout = {
        .slot = 0,
        .max_sets = image_count,
        // .entries = std::views::single(descriptr_entry),
        .entries = std::span<vk::descriptor_entry>(&descriptr_entry, 1),
        .descriptor_counts = std::views::single(max_descriptors),
    };

    vk::descriptor_resource set1(
      logical_device,
      set1_layout,
      vk::descriptor_layout_flags::update_after_bind_pool);

    // Retrieving Descriptor Set 1
    VkDescriptorSetLayout layout1 = set1.layout();

    std::array<vk::color_blend_attachment_state, 1> color_blend_attachments = {
        vk::color_blend_attachment_state{},
    };

    std::array<vk::dynamic_state, 2> dynamic_states = {
        vk::dynamic_state::viewport, vk::dynamic_state::scissor
    };

    uint32_t format = static_cast<uint32_t>(surface_properties.format.format);
    uint32_t vertex_mask = static_cast<uint32_t>(vk::shader_stage::vertex);
    uint32_t fragment_mask = static_cast<uint32_t>(vk::shader_stage::fragment);
    uint32_t stage_mask = vertex_mask | fragment_mask;
    vk::shader_stage stage = static_cast<vk::shader_stage>(stage_mask);
    vk::push_constant_range range = {
        .stage = stage,
        .offset = 0,
        .range = sizeof(push_constant_data),
    };

    vk::pipeline_params pipeline_configuration = {
        .use_render_pipeline = true,
        .color_attachment_formats = std::span<const uint32_t>(&format, 1),
        .depth_format = static_cast<uint32_t>(depth_format),
        .stencil_format = static_cast<uint32_t>(depth_format),
        .renderpass = nullptr,
        .shader_modules = geometry_resource.handles(),
        .vertex_attributes = geometry_resource.vertex_attributes(),
        .vertex_bind_attributes = geometry_resource.vertex_bind_attributes(),
        .descriptor_layouts = std::span<VkDescriptorSetLayout>(&layout1, 1),
        .color_blend = {
            .attachments = color_blend_attachments,
        },
        .depth_stencil_enabled = true,
        .dynamic_states = dynamic_states,
        .push_constants = std::views::single(range),
    };
    vk::pipeline main_graphics_pipeline(logical_device, pipeline_configuration);

    VkClearValue clear_color = {
        { 0.f, 0.5f, 0.5f, 1.f },
    };

    VkClearValue depth_value = {
        .depthStencil = { .depth = 1.f, .stencil = 0 },
    };

    // Testing grounds for render graph implementation

    // Load 3D model

    obj_model test_model(std::filesystem::path("asset_samples/viking_room.obj"),
                         logical_device,
                         physical_device);

    frame_graph render_graph;

    /*


    GBuffer (0) --->
                    | -> Lighting (2) --> UI (3) -> Final Output
    Shadow (0) --->

    */

    resource_storage storage(logical_device, physical_device);

    model_storage model_storages(logical_device, physical_device);

    // Setting up our GPU-resources (images)
    // Color Image (Geometry Pass)
    vk::image_params image_params = {};
    resource_id gbuffer_albedo = storage.add_image(
      "GBuffer_albedo",
      width,
      height,
      static_cast<vk::format>(surface_properties.format.format),
      vk::memory_property::device_local_bit,
      vk::image_usage::color_attachment_bit |
        vk::image_usage::transfer_src_bit);

    // Depth Image (Depth-Pass)
    resource_id gbuffer_depth_stencil = storage.add_image(
      "GBuffer_depth_stencil",
      width,
      height,
      static_cast<vk::format>(depth_format),
      vk::memory_property::device_local_bit,
      vk::image_usage::depth_stencil_bit,
      vk::image_aspect_flags::depth_bit | vk::image_aspect_flags::stencil_bit);

    resource_id gbuffer_normals = storage.add_image(
      "GBuffer_normals",
      width,
      height,
      vk::format::r16g16b16a16_sfloat,
      vk::memory_property::device_local_bit,
      vk::image_usage::color_attachment_bit | vk::image_usage::sampled_bit);

    // Lighting Output Image
    resource_id lighting_output_image = storage.add_image(
      "Lighting_output_image",
      width,
      height,
      static_cast<vk::format>(surface_properties.format.format),
      vk::memory_property::device_local_bit,
      vk::image_usage::color_attachment_bit | vk::image_usage::sampled_bit);

    // Handle Scene Uniforms
    resource_id scene_uniforms = storage.add_buffer<scene_ubo>(
      "scene_uniforms",
      vk::memory_property::host_visible_bit |
        vk::memory_property::host_cached_bit,
      vk::buffer_usage::uniform_buffer_bit |
        vk::buffer_usage::shader_device_address_bit);

    // Handle Model Matrices Array (uniforms)
    // NOTE: Make sure to set the max size of meshes in a given scene
    // uint32_t max_storage = sizeof(scene_ubo) * 10'000;
    resource_id model_matrix_uniforms =
      storage.add_buffer<sizeof(scene_ubo) * 10'000>(
        "ObjectsTable",
        vk::memory_property::host_visible_bit |
          vk::memory_property::host_cached_bit,
        vk::buffer_usage::uniform_buffer_bit |
          vk::buffer_usage::shader_device_address_bit);

    // Indirect Draw Buffer
    constexpr uint32_t max_draws = 10'000;
    resource_id indirect_buffer =
      storage.add_buffer<sizeof(VkDrawIndexedIndirectCommand) * max_draws>(
        "IndirectCommands",
        vk::memory_property::host_visible_bit |
          vk::memory_property::host_coherent_bit,
        vk::buffer_usage::indirect_buffer_bit);
    
    // Buffer that represents the stored metadata per draw batch (or mesh)
    resource_id draw_metadata_buffer =
      storage.add_buffer<sizeof(gpu_draw_info) * max_draws>(
        "DrawInfos",
        vk::memory_property::host_visible_bit |
          vk::memory_property::host_coherent_bit,
        vk::buffer_usage::storage_buffer_bit);
    
    std::array<resource_id, 3> gbuffer_writes = { gbuffer_albedo,
                                                  gbuffer_normals,
                                                  gbuffer_depth_stencil };

    // Setting up Scene Uniforms
    flecs::world registry;

    // Setup Camera stuff....
    flecs::entity camera = registry.entity("Camera");
    camera.add<flecs::pair<editor, projection_view>>();
    camera.set<perspective_camera>({
      .plane = { 0.1f, 5000.f },
      .is_active = true,
      .field_of_view = 45.f,
    });
    camera.set<transform>({
      .position = { 3.50f, 4.90f, 36.40f },
      .scale{ 1.f },
    });

    // Creating Viking Room
    flecs::entity viking_room = registry.entity("Viking Room");
    // viking_room.set<transform>({
    //     // .position = { -2.70f, 2.70, -8.30f },
    //     .position = {-2.70f, 2.70f, 1.f},
    //     // .rotation = { 2.30f, 95.90f, 91.80f },
    //     .rotation = {3.50f, 4.90f, 36.40f},
    //     .scale{ 1.f },
    // });
    // viking_room.set<transform>({
    //   .position = { -8.f, 1.79999995f, 1.5f },
    //   .rotation = { -25.0f, -20.0f, 0.0f },
    //   .scale{ 1.0f },
    // });
    viking_room.set<transform>({
      .position = { -3.0f, 1.5f, 5.0f },
      .scale = {1.f, 1.f, 1.f},
      .rotation = { -25.0f, -20.0f, 0.0f },
    });
    viking_room.set<mesh_source>({
      .model_path = "asset_samples/viking_room.obj",
      .diffuse_path = "asset_samples/viking_room.png",
    });

    flecs::entity cube = registry.entity("Cube");
    // cube.set<transform>({
    //   .position = { 2.20f, 3.40f, 28.0f },
    //   .rotation = { -25.0f, -20.0f, 0.0f },
    //   .scale{ 1.0f },
    // });
    cube.set<transform>({
      .position = { 3.0f, 1.5f, 5.0f },
      .scale = {1.f, 1.f, 1.f},
      .rotation = { -25.0f, -20.0f, 0.0f },
    });
    cube.set<mesh_source>({
      .model_path = "asset_samples/cube.obj",
      .diffuse_path = "asset_samples/container_diffuse.png",
    });

    flecs::entity robot = registry.entity("Robot");
    robot.set<transform>({
      .position = { -7.5f, 4.6f, -1.795f },
      .scale = {1.f, 1.f, 1.f},
      .rotation = { 2.9f, 16.5f, 0.6f },
    });
    robot.set<mesh_source>({
      .model_path = "asset_samples/robot.obj",
      .diffuse_path = "asset_samples/wood.png",
    });

    flecs::query<> all_meshes =
      registry.query_builder<mesh_source, transform>().build();

    std::vector<vk::texture> gpu_textures;
    std::unordered_map<std::string_view, gpu_material> material_table;
    uint32_t texture_slot = 1; // 0 = white texture

    vk::image_extent extent = { .width = 1, .height = 1 };
    std::array<uint8_t, 4> white_color = { 0xff, 0xff, 0xff, 0xff };
    gpu_textures.emplace_back(
      logical_device,
      extent,
      white_color,
      physical_device.memory_properties(vk::memory_property::host_visible_bit |
                                        vk::memory_property::host_cached_bit));

    scene_ubo scene_data = {};


    environment_map environment(logical_device, physical_device, "asset_samples/monkstown_castle_4k.hdr", surface_properties.format.format, depth_format);

    std::vector<VkDrawIndexedIndirectCommand> indirect_commands;
    std::vector<gpu_draw_info> draw_infos;

    glm::mat4 projection = glm::mat4(1.f);
    glm::mat4 view = glm::mat4(1.f);

    // GBUffer Pass
    render_graph.emplace_pass(
      "GBuffer",
      {},
      gbuffer_writes,
      [&](vk::command_buffer p_command, resource_storage& p_storage) {
          auto albedo_opt = p_storage.get_image("GBuffer_albedo");
          auto depth_opt = p_storage.get_image("GBuffer_depth_stencil");

          if (albedo_opt and depth_opt) {
              auto albedo = albedo_opt.value();
              auto depth = depth_opt.value();

              // Because dynamic rendering does not automatically handle layout
              // transitions These memory barriers set the color and depth
              // images for the output
              vk::v2::image_memory_barrier albedo_barrier = {
                    .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe, // Earliest stage: no prior work required
                    .src_access_mask = vk::v2::access_flags::memory_read_bit,     // Flushes generic prior reads
                    .dst_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output, // Waits at color output stage
                    .dst_access_mask = vk::v2::access_flags::color_attachment_write_bit, // Prepares for color attachment writes
                    .old_layout      = vk::image_layout::undefined, // Previous layout contents can be discarded
                    .new_layout      = vk::image_layout::color_optimal, // Transition to optimal color attachment layout
                    .subresource_range = {
                        .aspect_mask = vk::image_aspect_flags::color_bit, // Targets color-spaces
                        .level_count = 1,
                        .layer_count = 1,
                    },
                };
              albedo.pipeline_barriers(p_command,
                                       std::views::single(albedo_barrier));

              vk::v2::image_memory_barrier depth_barrier = {
                    .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe,
                    .src_access_mask = vk::v2::access_flags::memory_read_bit,
                    .dst_stage_mask  = vk::v2::pipeline_stage_flags::early_fragment_shader_bit,
                    .dst_access_mask = vk::v2::access_flags::depth_stencil_attachment_write_bit,
                    .old_layout      = vk::image_layout::undefined,
                    .new_layout      = vk::image_layout::depth_stencil_optimal,
                    .subresource_range = {
                        .aspect_mask = vk::image_aspect_flags::depth_bit | vk::image_aspect_flags::stencil_bit, // targets both depth and stencil aspects
                        .level_count = 1,
                        .layer_count = 1,
                    },
                };
              depth.pipeline_barriers(p_command,
                                      std::views::single(depth_barrier));

              vk::rendering_attachment color_render_attachment = {
                  .image_view = albedo.image_view(),
                  .layout = vk::image_layout::color_optimal,
                  .load = vk::attachment_load::clear,
                  .store = vk::attachment_store::store,
                  .clear_values = clear_color
              };

              vk::rendering_attachment depth_stencil_attachment = {
                  .image_view = depth.image_view(),
                  .layout = vk::image_layout::depth_stencil_optimal,
                  .load = vk::attachment_load::clear,
                  .store = vk::attachment_store::store,
                  .depth_values = depth_value
              };

              vk::rendering_begin_parameters begin_params = {
                  .render_area = { { 0, 0 },
                                   { static_cast<uint32_t>(width),
                                     static_cast<uint32_t>(height) } },
                  .layer_count = 1,
                  .color_attachments = std::views::single(color_render_attachment),
                  .depth_attachment = depth_stencil_attachment,
                  .stencil_attachment = depth_stencil_attachment,
              };

              p_command.begin_rendering(begin_params);
              main_graphics_pipeline.bind(p_command);
 
              std::optional<vk::dyn::buffer> objects_table_opt =
                p_storage.get_buffer("ObjectsTable");
              auto objects_table = objects_table_opt.value();
              objects_table.transfer<glm::mat4>(
                model_storages.model_matrices());

              all_meshes.each([&](flecs::entity p_entity) {
                  push_constant_data push = {
                      .texture_index =
                        material_table[p_entity.name().c_str()].diffuse_idx,
                      .model_matrix_id =
                        model_storages.get_index(p_entity.name().c_str()),
                      .scene_ubo_addr = p_storage.get_buffer("scene_uniforms")->get_device_address(),
                      .model_matrices_addr = objects_table.get_device_address(),
                  };

                  main_graphics_pipeline.push_constant<push_constant_data>(
                    p_command, push, stage, 0);

                  // 1.) Load
                  const auto& model_opt =
                    model_storages.get(p_entity.name().c_str());
                  if (!model_opt.has_value() or !model_opt->loaded()) {
                      std::println("Model {} did not load!",
                                   p_entity.get<mesh_source>()->model_path);
                      return;
                  }
                  auto obj_model = model_opt.value();
                  const VkBuffer vertex = obj_model.vertex_handle();
                  uint64_t offset = 0;
                  p_command.bind_vertex_buffers(std::views::single(vertex),
                                                std::views::single(offset));
                  if (obj_model.has_indices()) {
                      p_command.bind_index_buffers32(obj_model.index_handle());
                  }

                auto ind_buf_opt = p_storage.get_buffer("IndirectCommands");
                if(ind_buf_opt.has_value() && !indirect_commands.empty()) {
                  vkCmdDrawIndexedIndirect(
                    p_command,
                    ind_buf_opt.value(),
                    0,
                    static_cast<uint32_t>(indirect_commands.size()),
                    sizeof(VkDrawIndexedIndirectCommand));
                }
              });

              p_command.end_rendering();
          }
      });

    std::array<resource_id, 3> lighting_reads = { gbuffer_albedo,
                                                  gbuffer_normals,
                                                  gbuffer_depth_stencil };
    std::array<resource_id, 1> lighting_writes = { lighting_output_image };

    render_graph.emplace_pass(
      "Lighting_pass",
      lighting_reads,
      lighting_writes,
      [](vk::command_buffer p_command, resource_storage& p_storage) {});

    vk::sample_image* swapchain_img = nullptr;
    vk::sample_image* swapchain_depth_img = nullptr;
    std::array<resource_id, 2> present_reads = { gbuffer_albedo,
                                                 lighting_output_image };

    
    std::array<resource_id, 1> environment_reads = {gbuffer_depth_stencil};
    std::array<resource_id, 1> environment_writes = {gbuffer_albedo};
    render_graph.emplace_pass("environment_pass", environment_reads, environment_writes, [&](vk::command_buffer p_command, resource_storage& p_storage){

        auto albedo_opt = p_storage.get_image("GBuffer_albedo");
        auto depth_opt = p_storage.get_image("GBuffer_depth_stencil");
        environment.current_command(p_command);
        if(!albedo_opt and !depth_opt) {
            return;
        }

        auto albedo = albedo_opt.value();
        auto depth = depth_opt.value();

        vk::v2::image_memory_barrier albedo_barrier = {
            .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe, // Earliest stage: no prior work required
            .src_access_mask = vk::v2::access_flags::memory_read_bit,     // Flushes generic prior reads
            .dst_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output, // Waits at color output stage
            .dst_access_mask = vk::v2::access_flags::color_attachment_write_bit, // Prepares for color attachment writes
            .old_layout      = vk::image_layout::undefined, // Previous layout contents can be discarded
            .new_layout      = vk::image_layout::color_optimal, // Transition to optimal color attachment layout
            .subresource_range = {
                .aspect_mask = vk::image_aspect_flags::color_bit, // Targets color-spaces
                .level_count = 1,
                .layer_count = 1,
            },
        };
        albedo.pipeline_barriers(p_command,
                                std::views::single(albedo_barrier));

        vk::v2::image_memory_barrier depth_barrier = {
            .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe,
            .src_access_mask = vk::v2::access_flags::memory_read_bit,
            .dst_stage_mask  = vk::v2::pipeline_stage_flags::early_fragment_shader_bit,
            .dst_access_mask = vk::v2::access_flags::depth_stencil_attachment_write_bit,
            .old_layout      = vk::image_layout::undefined,
            .new_layout      = vk::image_layout::depth_stencil_optimal,
            .subresource_range = {
                .aspect_mask = vk::image_aspect_flags::depth_bit | vk::image_aspect_flags::stencil_bit,
                .level_count = 1,
                .layer_count = 1,
            },
        };
        depth.pipeline_barriers(p_command,
                                std::views::single(depth_barrier));

        vk::rendering_attachment color_render_attachment = {
            .image_view = albedo.image_view(),
            .layout = vk::image_layout::color_optimal,
            .load = vk::attachment_load::load,
            .store = vk::attachment_store::store,
            .clear_values = clear_color
        };

        vk::rendering_attachment depth_stencil_attachment = {
            .image_view = depth.image_view(),
            .layout = vk::image_layout::depth_stencil_optimal,
            .load = vk::attachment_load::load,
            .store = vk::attachment_store::store,
            .depth_values = depth_value
        };

        vk::rendering_begin_parameters begin_params = {
            .render_area = { { 0, 0 },
                            { static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height) } },
            .layer_count = 1,
            .color_attachments = std::views::single(color_render_attachment),
            .depth_attachment = depth_stencil_attachment,
            .stencil_attachment = depth_stencil_attachment,
        };

        p_command.begin_rendering(begin_params);

        environment.begin(projection, view);

        environment.end();

        p_command.end_rendering();
    });
    // Final Output Pass
    /**

    NOTE:

    GBUffer -> Final Output


    With Lighting (this is due to an additional dependecy of read ocurring from
    the lighting pass): GBuffer -> Lighting -> Final Output

    */
    render_graph.emplace_pass(
      "Final Output",
      present_reads,
      {},
      [&](vk::command_buffer p_command, resource_storage& p_storage) {
          if (swapchain_img == nullptr and swapchain_depth_img == nullptr) {
              return;
          }

          auto albedo_opt = p_storage.get_image("GBuffer_albedo");
          VkFormat swapchain_format = surface_properties.format.format;

          if (albedo_opt) {
              auto albedo = albedo_opt.value();

              vk::v2::image_memory_barrier albedo_transfer_barrier = {
                  .src_stage_mask =
                    vk::v2::pipeline_stage_flags::color_attachment_output,
                  .src_access_mask =
                    vk::v2::access_flags::color_attachment_write_bit,
                  .dst_stage_mask = vk::v2::pipeline_stage_flags::transient_bit,
                  .dst_access_mask = vk::v2::access_flags::transfer_read_bit,
                  .old_layout = vk::image_layout::color_optimal,
                  .new_layout = vk::image_layout::transfer_src_optimal,
                  .subresource_range = { .aspect_mask =
                                           vk::image_aspect_flags::color_bit },
              };
              albedo.pipeline_barriers(
                p_command, std::views::single(albedo_transfer_barrier));
              vk::v2::image_memory_barrier swapchain_dst_barrier = {
                  .src_stage_mask = vk::v2::pipeline_stage_flags::top_of_pipe,
                  .src_access_mask = vk::v2::access_flags::memory_read_bit,
                  .dst_stage_mask = vk::v2::pipeline_stage_flags::transient_bit,
                  .dst_access_mask = vk::v2::access_flags::transfer_write_bit,
                  .old_layout = vk::image_layout::undefined,
                  .new_layout = vk::image_layout::transfer_dst_optimal,
                  .subresource_range = { .aspect_mask =
                                           vk::image_aspect_flags::color_bit },
              };
              swapchain_img->pipeline_barriers(
                p_command, std::views::single(swapchain_dst_barrier));

              vk::image_targets target = {
                  .src = { .image = albedo,
                           .layout = static_cast<VkImageLayout>(
                             vk::image_layout::transfer_src_optimal) },
                  .dst = { .image = *swapchain_img,
                           .layout = static_cast<VkImageLayout>(
                             vk::image_layout::transfer_dst_optimal) },
              };

              vk::image_blit blit = {
                        .src = {
                            .aspect_mask = vk::image_aspect_flags::color_bit,
                            .mip_level = 0,
                            .level_count = 0,
                            .layer_count = 1,
                        },
                        .src_offsets = {
                            VkOffset3D{0, 0, 0},
                            VkOffset3D{width, height, 1},
                        },
                        .dst = {
                            .aspect_mask = vk::image_aspect_flags::color_bit,
                            .mip_level = 0,
                            .level_count = 0,
                            .layer_count = 1,
                        },
                        .dst_offsets = {
                            VkOffset3D{0, 0, 0},
                            VkOffset3D{static_cast<int32_t>(swapchain_extent.width), static_cast<int32_t>(swapchain_extent.height), 1},
                        },
                    };
              p_command.blit_image(target, std::views::single(blit));

              vk::v2::image_memory_barrier swapchain_present_barrier = {
                  .src_stage_mask = vk::v2::pipeline_stage_flags::transient_bit,
                  .src_access_mask = vk::v2::access_flags::transfer_write_bit,
                  .dst_stage_mask =
                    vk::v2::pipeline_stage_flags::bottom_of_pipe,
                  .dst_access_mask = vk::v2::access_flags::memory_read_bit,
                  .old_layout = vk::image_layout::transfer_dst_optimal,
                  .new_layout = vk::image_layout::present_src_khr,
                  .subresource_range = { .aspect_mask =
                                           vk::image_aspect_flags::color_bit },
              };
              swapchain_img->pipeline_barriers(
                p_command, std::views::single(swapchain_present_barrier));
          }
      });

    
      render_graph.compile();

    render_graph.print_dependencies();

    // Querying all editor camera
    auto query_editor_camera =
      registry
        .query_builder<flecs::pair<editor, projection_view>,
                       perspective_camera>()
        .build();
    glm::vec4 camera_position{ 1.f };

    float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
    registry
      .system<flecs::pair<editor, projection_view>,
              transform,
              perspective_camera>()
      .each([&](flecs::pair<editor, projection_view> p_pair,
                transform& p_transform,
                perspective_camera& p_camera) {
          if (!p_camera.is_active) {
              return;
          }

          p_pair->projection =
            glm::perspective(glm::radians(p_camera.field_of_view),
                             aspect_ratio,
                             p_camera.plane.x,
                             p_camera.plane.y);
          p_pair->projection[1][1] *= -1;

          p_pair->view = glm::mat4(1.f);

          // This is converting a glm::highp_vec4 to a glm::quat
          glm::quat quaternion = to_quat(p_transform.quaternion);

          p_pair->view = glm::translate(p_pair->view, p_transform.position) *
                         glm::mat4_cast(quaternion);

          p_pair->view = glm::inverse(p_pair->view);
          projection = p_pair->projection;
          view = p_pair->view;
      });
    auto start_time = std::chrono::high_resolution_clock::now();
    float delta_time = 0.f;

    // Loading 3D meshes
    all_meshes.each([&](flecs::entity p_entity) {
        const transform* t = p_entity.get<transform>();
        const mesh_source* src = p_entity.get<mesh_source>();

        if (!model_storages.contains(p_entity.name().c_str())) {
            if (model_storages.contains(p_entity.name().c_str())) {
                return;
            }
            model_storages.add_model(p_entity.name().c_str(),
                                     std::filesystem::path(src->model_path));
            model_storages.write_model(p_entity.name().c_str(), *t);

            vk::texture_params config_texture = {
                .memory_mask = physical_device.memory_properties(
                  vk::memory_property::host_visible_bit |
                  vk::memory_property::host_cached_bit),
            };
            // Loading texture and setting up VkSampler and VkImageView
            stb_image diffuse_img =
              stb_image(src->diffuse_path, config_texture);
            stb_image specular_img =
              stb_image(src->specular_path, config_texture);

            // Loading Diffuse Texture
            gpu_material material = {};
            if (!src->diffuse_path.empty()) {
                material.diffuse_idx = texture_slot++;
                gpu_textures.emplace_back(
                  logical_device, &diffuse_img, config_texture);
            }

            // Loading Specular Texture
            if (!src->specular_path.empty()) {
                material.specular_idx = texture_slot++;
                gpu_textures.emplace_back(
                  logical_device, &specular_img, config_texture);
            }

            material_table.emplace(p_entity.name().c_str(), material);
        }
    });

    // Setup image loading
    vk::texture_params texture_params = {
        .memory_mask = physical_device.memory_properties(
          vk::memory_property::host_visible_bit |
          vk::memory_property::host_cached_bit),
    };
    stb_image image =
      stb_image("asset_samples/viking_room.png", texture_params);
    vk::texture viking_room_texture(logical_device, &image, texture_params);

    // Setting up writing via Descriptors
    std::vector<vk::write_image> gpu_write_desc_images;
    gpu_write_desc_images.reserve(gpu_textures.size());

    for (auto& image : gpu_textures) {
        vk::write_image write_texture_image = {
            .sampler = image.image().sampler(),
            .view = image.image().image_view(),
            .layout = vk::image_layout::shader_read_only_optimal,
        };
        gpu_write_desc_images.emplace_back(write_texture_image);
    }

    vk::write_image_descriptor set1_sampler = {
        .dst_binding = 1,
        .sample_images = gpu_write_desc_images,
    };

    set1.update({}, std::views::single(set1_sampler));

    // Setting up Draw Indexed Indirect Commands in this each
    // vkCmdDrawIndexedIndirect gets called in emplace_pass("GBuffer",...)
    all_meshes.each([&](flecs::entity p_entity){
        std::string name(p_entity.name().c_str());
        const auto model_opt = model_storages.get(name);
        if(!model_opt.has_value()) {
            std::println("Error: Model {} was not found in model_storage!", name);
            return;
        }

        uint32_t matrix_idx = model_storages.get_index(name);
        const auto& mat = material_table[name];

        indirect_commands.push_back(VkDrawIndexedIndirectCommand{
            .indexCount = model_storages.indices_size(name),
            .instanceCount = 1,
            .firstIndex = 0,
            .vertexOffset = 0,
            .firstInstance = matrix_idx,
        });

        draw_infos.push_back(gpu_draw_info{
            .model_matrix_index = matrix_idx,
            .diffuse_idx = mat.diffuse_idx,
            .specular_idx = mat.specular_idx,
        });
    });

    if (!indirect_commands.empty()) {
        auto indirect_buf_opt = storage.get_buffer("IndirectCommands");
        if(indirect_buf_opt.has_value()) {
            auto indirect_buf = indirect_buf_opt.value();
            indirect_buf.transfer<VkDrawIndexedIndirectCommand>(indirect_commands);
        }
    }

    if(!draw_infos.empty()) {
        auto draw_infos_buf_opt = storage.get_buffer("DrawInfos");
        if(draw_infos_buf_opt.has_value()) {
            auto draw_infos_buf = draw_infos_buf_opt.value();
            draw_infos_buf.transfer<gpu_draw_info>(draw_infos);
        }
    }

    bool is_first_frame = true;
    glm::vec2 last_cursor_pos=glm::vec2(0.f);
    float yaw = 0.f;
    float pitch = 0.f;

    while (!glfwWindowShouldClose(window)) {
        uint32_t current_frame = presentation_queue.acquire_next_image();
        vk::command_buffer current = swapchain_command_buffers[current_frame];

        swapchain_img = &swapchain_images[current_frame];
        swapchain_depth_img = &swapchain_depth_images[current_frame];

        auto current_time = std::chrono::high_resolution_clock::now();

        delta_time = std::chrono::duration<float, std::chrono::seconds::period>(
                       current_time - start_time)
                       .count();

        start_time = current_time;

        if (delta_time > 0.1f) {
            delta_time = 0.1f;
        }

        glfwPollEvents();

        registry.progress(delta_time);


        // Perform Physics Update Operation (ignore)

        // Perform Update Operation (transforms)

        // Process camera stuff
        // glm::mat4 projection{ 1.f };
        // glm::mat4 view{ 1.f };

        // Inputting Camera Update Logic Here
        transform* t = camera.get_mut<transform>();
        float dt = delta_time;
        float default_speed = 10.f;
        float movement_speed = 10.f;
        float velocity = default_speed * dt;
        if (is_mouse_pressed(window, mouse_button_middle)) {
            velocity = movement_speed * dt;
        }

        glm::quat to_quaternion = to_quat(t->quaternion);

        glm::vec3 up = glm::rotate(to_quaternion, math::up());
        glm::vec3 forward = glm::rotate(to_quaternion, math::backward());
        glm::vec3 right = glm::rotate(to_quaternion, math::right());

        // Q = UP
        if (is_key_pressed(window, key_q)) {
            t->position += up * velocity;
        }

        // E = DOWN
        if (is_key_pressed(window, key_e)) {
            t->position -= up * velocity;
        }

        if (is_key_pressed(window, key_w)) {
            t->position += forward * velocity;
        }
        if (is_key_pressed(window, key_s)) {
            t->position -= forward * velocity;
        }

        if (is_key_pressed(window, key_d)) {
            t->position += right * velocity;
        }
        if (is_key_pressed(window, key_a)) {
            t->position -= right * velocity;
        }

        glm::vec2 current_cursor_pos = cursor_position(window);
        if (is_first_frame) {
            last_cursor_pos = current_cursor_pos;
            is_first_frame = false;
        }

        // NOTE: For now disabling, will add back later...
        // if (is_mouse_pressed(window, mouse_button_middle)) {
        //     // set_cursor_mode(atlas::cursor_mode::disabled);
        //     // sensitivity is how fast the cursor rotates
        //     float mouse_sensitivity = 0.01;
        //     glm::vec2 cursor_dt = current_cursor_pos - last_cursor_pos;

        //     yaw -= (cursor_dt.x * mouse_sensitivity);
        //     pitch -= (cursor_dt.y * mouse_sensitivity);

        //     t->set_rotation(glm::vec3(pitch, yaw, 0.0f));
        // }
        float turn_speed = 0.5f;

        // Inside your while loop:
        if (is_key_pressed(window, key_o)) {
            yaw += turn_speed * dt;
        }
        if (is_key_pressed(window, key_p)) {
            yaw -= turn_speed * dt;
        }

        t->set_rotation(glm::vec3(pitch, yaw, 0.0f));

        query_editor_camera.each(
          [&](flecs::entity p_entity,
              flecs::pair<editor, projection_view> p_pair,
              perspective_camera& p_camera) {
              if (!p_camera.is_active) {
                  return;
              }

              // Update Cam position to be written into the shader
              const transform* t = p_entity.get<transform>();
              camera_position = glm::vec4(t->position, 1.f);
              projection = p_pair->projection;
              view = p_pair->view;
          });


        current.begin(vk::command_usage::simulatneous_use_bit);

        vk::viewport_params viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };
        current.set_viewport(std::views::single(viewport), 0);

        vk::scissor_params scissor = {
            .offset = { 0, 0 },
            .extent = { static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height) },
        };
        current.set_scissor(std::views::single(scissor), 0);

        scene_data.view = view;
        scene_data.proj = projection;
        scene_data.camera_pos = camera_position;
        std::optional<vk::dyn::buffer> scene_optional = storage.get_buffer("scene_uniforms");
        scene_optional->transfer<scene_ubo>(std::views::single(scene_data));
        

        // Perform Late Update Operation (ignore)
        current.bind_descriptors(
          main_graphics_pipeline.layout(),
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          std::views::single(static_cast<VkDescriptorSet>(set1)));
        render_graph.execute(current, storage);

        current.end();

        // Submitting and then presenting to the screen
        std::array<const VkCommandBuffer, 1> commands = { current };
        presentation_queue.submit_async(commands);
        presentation_queue.present_frame(current_frame);
    }

    logical_device.wait();
    main_swapchain.destruct();

    geometry_resource.destruct();
    main_graphics_pipeline.destruct();
    set1.destruct();
    viking_room_texture.destruct();
    test_model.destruct();
    model_storages.destruct();
    for (auto& texture : gpu_textures) {
        texture.destruct();
    }

    environment.destruct();

    for (auto& command : swapchain_command_buffers) {
        command.destruct();
    }

    for (auto& image : swapchain_images) {
        image.destruct();
    }

    for (auto& image : swapchain_depth_images) {
        image.destruct();
    }

    storage.destroy();

    presentation_queue.destruct();

    logical_device.destruct();
    window_surface.destruct();

    glfwDestroyWindow(window);
    return 0;
}