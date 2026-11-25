module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdint>


export module events;


export import :keyboard;
export import :mouse;


export {
    bool is_key_pressed(GLFWwindow* p_window, int p_key) {

            auto state = glfwGetKey(p_window, static_cast<int32_t>(p_key));
            return (state == GLFW_PRESS);
        }

        bool is_key_released(GLFWwindow* p_window, int p_key) {

            auto state = glfwGetKey(p_window, static_cast<int32_t>(p_key));
            return (state == GLFW_RELEASE);
        }

        bool is_mouse_pressed(GLFWwindow* p_window, int p_mouse_code) {

            auto state =
              glfwGetMouseButton(p_window, static_cast<int32_t>(p_mouse_code));
            return (state == GLFW_PRESS);
        }

        bool is_mouse_released(GLFWwindow* p_window, int p_mouse_code) {

            auto state =
              glfwGetMouseButton(p_window, static_cast<int32_t>(p_mouse_code));
            return (state == GLFW_RELEASE);
        }

        glm::vec2 cursor_position(GLFWwindow* p_window) {

            double x_pos, y_pos;
            glfwGetCursorPos(p_window, &x_pos, &y_pos);

            return { x_pos, y_pos };
        }
};