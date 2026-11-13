module;

#include <cstdint>
#include <variant>
#include <string>
#include <functional>

export module types;

import vk;

export {
    using resource_id = uint64_t;
    using pass_id = uint64_t;

    struct image_resource {
        uint32_t width=0;
        uint32_t height=0;
        uint32_t mip_levels=1;
        vk::format format;
    };

    struct buffer_resource {
        uint64_t size_bytes;
        uint32_t stride=0;
        bool is_storage=false; // e.g., for compute buffering BDA
    };

    struct resource_description {
        std::string name="N/A";
        resource_id id=0;
        std::variant<buffer_resource, image_resource> resource;
        int first_pass=-1;
        int last_pass=-1;
    };

    struct pass_description {
        pass_description(std::string_view p_name, pass_id p_id, std::span<const resource_id> p_reads, std::span<const resource_id> p_writes) :
                name(p_name), id(p_id), reads(p_reads.begin(), p_reads.end()), writes(p_writes.begin(), p_writes.end()){

        }
        std::string name;
        pass_id id;
        std::vector<resource_id> reads;
        std::vector<resource_id> writes;
    };
}