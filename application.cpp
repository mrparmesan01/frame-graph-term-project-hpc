#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <print>
#include <array>
#include <ranges>
#include <expected>

import vk;
import vk_utilities;
import types;
import frame_graph;

int main() {

    if(!glfwInit()) {
        std::println("GLFW Error: glfwInit could not be initialized!\n");
        return -1;
    }

    if(!glfwVulkanSupported()) {
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
        .validations =
          validation_layers,
        .extensions =
          global_extensions
    };

    // 1. Setting up vk instance
    vk::instance api_instance(config, debug_callback_info);

    if(api_instance.alive()) {
        std::println("vk::instance is initialized!!");
    }

#if __APPLE__
    std::expected<vk::physical_device, VkResult> res = api_instance.enumerate_physical_device(vk::physical_gpu::integrated);
#else
    std::expected<vk::physical_device, VKResult> res = api_instance.enumerate_physical_device(vk::physical_gpu::discrete);
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
    VkFormat depth_format = physical_device.request_depth_format(format_support);

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
    vk::swapchain main_swapchain(logical_device,
                                 window_surface,
                                 enumerate_swapchain_settings,
                                 surface_properties);

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
            .usage = vk::image_usage::color_attachment_bit,
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

    std::array<vk::color_blend_attachment_state, 1> color_blend_attachments = {
        vk::color_blend_attachment_state{},
    };

    std::array<vk::dynamic_state, 2> dynamic_states = {
        vk::dynamic_state::viewport, vk::dynamic_state::scissor
    };

    uint32_t format = static_cast<uint32_t>(surface_properties.format.format);
    vk::pipeline_params pipeline_configuration = {
        .use_render_pipeline = true,
        .color_attachment_formats = std::span<const uint32_t>(&format, 1),
        .depth_format = static_cast<uint32_t>(depth_format),
        .stencil_format = static_cast<uint32_t>(depth_format),
        .renderpass = nullptr,
        .shader_modules = geometry_resource.handles(),
        .vertex_attributes = geometry_resource.vertex_attributes(),
        .vertex_bind_attributes = geometry_resource.vertex_bind_attributes(),
        .color_blend = {
            .attachments = color_blend_attachments,
        },
        .depth_stencil_enabled = true,
        .dynamic_states = dynamic_states,
    };
    vk::pipeline main_graphics_pipeline(logical_device, pipeline_configuration);

    VkClearValue clear_color = {
        { 0.f, 0.5f, 0.5f, 1.f },
    };

    VkClearValue depth_value = {
        .depthStencil = { .depth = 1.f, .stencil = 0 },
    };



    // Testing grounds for render graph implementation






    frame_graph render_graph;
    /*
    resource_storage storage;
    // render_graph.add_image("color_attachment (image)", swapchain_extent.width, swapchain_extent.height);
    // render_graph.add_image("depth_attachment (image)", swapchain_extent.width, swapchain_extent.height, true);
    // render_graph.add_buffer("vertex (buffer)", 1024);
    // render_graph.allocate();
    resource_id gbuffer_color_res = storage.add_image("GBuffer Color", swapchain_extent.width, swapchain_extent.height, static_cast<vk::format>(surface_properties.format.format));
    resource_id gbuffer_depth_res = storage.add_image("GBuffer Depth", swapchain_extent.width, swapchain_extent.height, static_cast<vk::format>(depth_format));
    resource_id shadow_map_res   = storage.add_image("Shadow Map", 2048, 2048, static_cast<vk::format>(depth_format));
    resource_id final_color_res  = storage.add_image("Final Swapchain Color", swapchain_extent.width, swapchain_extent.height, static_cast<vk::format>(surface_properties.format.format));

    // 2. Add Shadow Pass (Root pass: writes shadow map)
    render_graph.emplace_pass(
        "Shadow Pass",
        {},                            // Reads: None
        // { shadow_map_res },            // Writes: Shadow map
        std::views::single(shadow_map_res),
        [&](vk::command_buffer cmd) {
            std::println("Executing Shadow Pass...");
            // Record shadow pass commands here
        }
    );

    // 3. Add GBuffer Pass (Root pass: writes geometry info / color / depth)
    std::array<resource_id, 2> geo = {gbuffer_color_res, gbuffer_depth_res};
    render_graph.emplace_pass(
        "GBuffer",
        {},                            // Reads: None
        geo, // Writes: GBuffer targets
        [&](vk::command_buffer cmd) {
            std::println("Executing GBuffer Pass...");
            // Record GBuffer geometry pass here using main_graphics_pipeline
        }
    );

    // 4. Add Lighting Pass (Depends on GBuffer and Shadow Pass)
    std::array<resource_id, 3> lighting_reads = {gbuffer_color_res, gbuffer_depth_res, shadow_map_res};
    std::array<resource_id, 1> lighting_writes = {final_color_res};
    render_graph.emplace_pass(
        "Lighting",
        lighting_reads, // Reads: GBuffer & Shadows
        lighting_writes,                                      // Writes: Shaded output
        [&](vk::command_buffer cmd) {
            std::println("Executing Lighting Pass...");
            // Record lighting compute/fullscreen pass here
        }
    );

    // 5. Add UI Pass (Depends on Lighting Pass output)
    std::array<resource_id, 1> ui_reads = {final_color_res};
    std::array<resource_id, 1> ui_writes = {final_color_res};

    render_graph.emplace_pass(
        "UI Pass",
        ui_reads,           // Reads: Lit image
        ui_writes,           // Writes: Blends UI over final image
        [&](vk::command_buffer cmd) {
            std::println("Executing UI Pass...");
            // Record UI rendering commands here
        }
    );
    */

    /*
        
    
    GBuffer (0) --->
            | -> Lighting (2) --> UI (3)
    Shadow (0) --->
    
    
    
    */
    std::array<uint32_t,1> gbuffer_reads = {1};
    render_graph.emplace_pass("GBuffer", {}, gbuffer_reads);

    std::array<uint32_t, 1> lighting_reads = {3};
    render_graph.emplace_pass("Lighting", {}, lighting_reads);

    std::array<uint32_t, 1> shadow_reads = {1};
    render_graph.emplace_pass("Shadow Pass", {}, shadow_reads);

    std::array<uint32_t, 1> ui_writes = {1};
    render_graph.emplace_pass("UI Pass", ui_writes, {});

    render_graph.compile();

    render_graph.print_dependencies();




    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        uint32_t current_frame = presentation_queue.acquire_next_image();
        vk::command_buffer current = swapchain_command_buffers[current_frame];

        current.begin(vk::command_usage::simulatneous_use_bit);

        // Because dynamic rendering does not automatically handle layout
        // transitions These memory barriers set the color and depth images for
        // the output

        // pipeline barrier for color attachment to transition from undefined to
        // color optimal layout
        vk::v2::image_memory_barrier color_memory_barrier = {
            .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe,
            .src_access_mask = vk::v2::access_flags::memory_read_bit,
            .dst_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output,
            .dst_access_mask = vk::v2::access_flags::color_attachment_write_bit,
            .old_layout = vk::image_layout::undefined,
            .new_layout = vk::image_layout::color_optimal,
            .subresource_range = {
                .aspect_mask = vk::image_aspect_flags::color_bit,
            },
        };
        swapchain_images[current_frame].pipeline_barriers(
          current, std::views::single(color_memory_barrier));

        // pipeline barrier for depth attachment to transition from undefined to
        // depth optimal layout
        vk::v2::image_memory_barrier image_depth_memory_barrier = {
            .src_stage_mask  = vk::v2::pipeline_stage_flags::top_of_pipe,
            .src_access_mask = vk::v2::access_flags::memory_read_bit,
            .dst_stage_mask  = vk::v2::pipeline_stage_flags::early_fragment_shader_bit,
            .dst_access_mask = vk::v2::access_flags::depth_stencil_attachment_write_bit,
            .old_layout = vk::image_layout::undefined,
            .new_layout = vk::image_layout::depth_stencil_optimal,
            .subresource_range = {
                .aspect_mask = vk::image_aspect_flags::depth_bit | vk::image_aspect_flags::stencil_bit,
            },
        };
        swapchain_depth_images[current_frame].pipeline_barriers(
          current, std::views::single(image_depth_memory_barrier));

        vk::rendering_attachment color_render_attachment = {
            .image_view = swapchain_images[current_frame].image_view(),
            .layout = vk::image_layout::color_optimal,
            .resolve_mode = vk::resolved_mode_flags::none,
            .resolve_image_view = nullptr,
            .resolve_image_layout = vk::image_layout::undefined,
            .load = vk::attachment_load::clear,
            .store = vk::attachment_store::store,
            .clear_values = clear_color
        };

        vk::rendering_attachment depth_stencil_attachment = {
            .image_view = swapchain_depth_images[current_frame].image_view(),
            .layout = vk::image_layout::depth_stencil_optimal,
            .resolve_mode = vk::resolved_mode_flags::none,
            .resolve_image_view = nullptr,
            .resolve_image_layout = vk::image_layout::undefined,
            .load = vk::attachment_load::clear,
            .store = vk::attachment_store::store,
            .depth_values = depth_value
        };

        vk::rendering_begin_parameters begin_params = {
            .render_area = { { 0, 0 },
                             {
                               swapchain_extent.width,
                               swapchain_extent.height,
                             }, },
            .layer_count = 1,
            .color_attachments = std::span<const vk::rendering_attachment>(
              &color_render_attachment, 1),
            .depth_attachment = depth_stencil_attachment,
            .stencil_attachment = depth_stencil_attachment,
        };

        vk::viewport_params viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapchain_extent.width),
            .height = static_cast<float>(swapchain_extent.height),
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };
        current.set_viewport(
          0, 1, std::span<const vk::viewport_params>(&viewport, 1));

        vk::scissor_params scissor = {
            .offset = { 0, 0 },
            .extent = swapchain_extent,
        };

        current.set_scissor(
          0, 1, std::span<const vk::scissor_params>(&scissor, 1));

        current.begin_rendering(begin_params);

        main_graphics_pipeline.bind(current);
        vkCmdDraw(current, 3, 1, 0, 0);

        current.end_rendering();

        // pipeline barrier for color attachment to transition from color
        // optimal to present src layout reusing image_memory_barrier variable
        // to set the new layout transition for the color attachment
        color_memory_barrier = {
            .src_stage_mask  = vk::v2::pipeline_stage_flags::color_attachment_output,
            .src_access_mask = vk::v2::access_flags::color_attachment_write_bit,
            .dst_stage_mask  = vk::v2::pipeline_stage_flags::bottom_of_pipe,
            .dst_access_mask = vk::v2::access_flags::memory_read_bit,
            .old_layout = vk::image_layout::color_optimal,
            .new_layout = vk::image_layout::present_src_khr,
            .subresource_range = {
                .aspect_mask = vk::image_aspect_flags::color_bit,
            },
        };
        swapchain_images[current_frame].pipeline_barriers(
          current, std::views::single(color_memory_barrier));

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

    for (auto& command : swapchain_command_buffers) {
        command.destruct();
    }

    for (auto& image : swapchain_images) {
        image.destruct();
    }

    for (auto& image : swapchain_depth_images) {
        image.destruct();
    }

    presentation_queue.destruct();

    logical_device.destruct();
    window_surface.destruct();

    glfwDestroyWindow(window);
    return 0;
}