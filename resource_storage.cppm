module;

#include <vulkan/vulkan.h>
#include <string_view>
#include <unordered_map>
#include <map>

export module resource_storage;

import vk;
import types;


export {
/**
* @brief Stores GPU-transient resources such as images and buffers
* Performs resource look-ups via resource_id
*/
class resource_storage {
public:

    resource_storage() = delete;

    resource_storage(vk::device p_device, vk::physical_device p_physical) : m_device(p_device), m_physical(p_physical) {}

    ~resource_storage() = default;

    /**
     * @brief 
     * 
     * @param p_name 
     * @param p_width 
     * @param p_height 
     * @param p_format 
     * @param p_property_flags 
     * @return resource_id 
     *
     * Example Usage:
     * ```C++
     * storage.add_image("HDRI", 1024, 1024, surface.format.format, vk::memory_property::device_local_bit);
     */
    resource_id add_image(std::string_view p_name, uint32_t p_width, uint32_t p_height, vk::format p_format, vk::memory_property p_property_flags, vk::image_usage p_usage, vk::image_aspect_flags p_aspect=vk::image_aspect_flags::color_bit) {
        resource_id id = m_current_resource_id++;

        VkFormat format = static_cast<VkFormat>(p_format);
        vk::image_params image_params = {
            .extent = { .width = p_width,
                        .height = p_height },
            .format = format,
            // .memory_mask = m_physical.memory_properties(
            //   vk::memory_property::device_local_bit),
            .memory_mask = m_physical.memory_properties(p_property_flags),
            .aspect = p_aspect,
            .usage = p_usage,
            .mip_levels = 1,
            .layer_count = 1,
        };

        m_resource_table[p_name] = id;
        m_images.try_emplace(id, m_device, image_params);

        return id;
    }

    /**
     * @brief Adds a buffer to the 
     * 
     * 
     * @tparam T 
     * @param p_name 
     * @return resource_id 
     * 
     * Example Usage:
     * ```C++
     * storage.add_buffer("PBR", vk::memory_property::device_local_bit | vk::memory_property::host_visible_bit, vk::buffer_usage::transfer_dst_bit);
     */
    template<size_t T>
    resource_id add_buffer(std::string_view p_name, vk::memory_property p_property_flags, vk::buffer_usage p_usage) {
        resource_id id = m_current_resource_id++;

        // .usage = vk::buffer_usage::uniform_buffer_bit |
        //          vk::buffer_usage::shader_device_address_bit,
        // .allocate_flags = vk::memory_allocate_flags::device_address_bit_khr,

        vk::buffer_parameters buffer_params = {
            // .memory_mask = m_physical.memory_properties(
            //   vk::memory_property::device_local_bit |
            //   vk::memory_property::host_visible_bit),
            .memory_mask = m_physical.memory_properties(p_property_flags),
            // .usage = vk::buffer_usage::transfer_dst_bit |
            //          vk::buffer_usage::vertex_buffer_bit,
            .usage = p_usage | vk::buffer_usage::shader_device_address_bit,
            .allocate_flags = vk::memory_allocate_flags::device_address_bit_khr,
        };

        m_resource_table[p_name] = id;

        m_buffers.try_emplace(id, m_device, sizeof(T), buffer_params);
        
        return id;
    }

    /**
     * @brief Get the buffer object
     * 
     * Example Usage:
     * ```C++
     * auto opt = storage.get_buffer("PBR");
     ```
     * 
     * @param p_name 
     * @return std::optional<vk::buffer> 
     */
    std::optional<vk::dyn::buffer> get_buffer(std::string_view p_name) {

        resource_id id = -1;

        if(m_resource_table.contains(p_name)) {
            id = m_resource_table[p_name];
        }

        if(m_buffers.contains(id)) {
            return m_buffers[id];
        }

        return {};
    }

    std::optional<vk::sample_image> get_image(std::string_view p_name) {
        resource_id id = -1;
        if(m_resource_table.contains(p_name)) {
            id = m_resource_table[p_name];
        }

        if(m_images.contains(id)) {
            return m_images[id];
        }

        return {};
    }

    void destroy() {
        for(auto&[key, img] : m_images) {
            img.destruct();
        }

        for(auto&[key, buf] : m_buffers) {
            buf.reset();
        }
    }

private:
    vk::device m_device;
    vk::physical_device m_physical;

    resource_id m_current_resource_id=0;

    std::unordered_map<resource_id, vk::sample_image> m_images;

    std::unordered_map<resource_id, vk::dyn::buffer> m_buffers;

    // <name, resource_id> for looking up for specific resource to retrieve the specific resources.
    std::unordered_map<std::string_view, resource_id> m_resource_table;
};
};