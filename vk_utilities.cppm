module;

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

#include <vector>
#include <print>

export module vk_utilities;

import vk;


export {
    VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT p_message_severity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT p_message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    [[maybe_unused]] void* p_user_data) {

        std::print("validation layer:\t\t{}\n\n", p_callback_data->pMessage);
        return false;
    }

    std::vector<const char*>
    get_instance_extensions() {
        std::vector<const char*> extension_names;
        uint32_t extension_count = 0;
        const char** required_extensions =
        glfwGetRequiredInstanceExtensions(&extension_count);

        for (uint32_t i = 0; i < extension_count; i++) {
            std::println("Required Extension = {}", required_extensions[i]);
            extension_names.emplace_back(required_extensions[i]);
        }

        extension_names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    #if defined(__APPLE__)
        extension_names.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extension_names.emplace_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    #endif

        return extension_names;
    }
}