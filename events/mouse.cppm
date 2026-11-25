module;

#include <cstdint>

export module events:mouse;


export {
    enum Mouse : uint32_t {
        // From glfw3.h
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Button6 = 6,
        Button7 = 7,

        ButtonLast = Button7,
        ButtonLeft = Button0,
        ButtonRight = Button1,
        ButtonMiddle = Button2
    };

    inline constexpr uint32_t mouse_button_0 = Mouse::Button0;
    inline constexpr uint32_t mouse_button_1 = Mouse::Button1;
    inline constexpr uint32_t mouse_button_2 = Mouse::Button2;
    inline constexpr uint32_t mouse_button_3 = Mouse::Button3;
    inline constexpr uint32_t mouse_button_4 = Mouse::Button4;
    inline constexpr uint32_t mouse_button_5 = Mouse::Button5;
    inline constexpr uint32_t mouse_button_6 = Mouse::Button6;
    inline constexpr uint32_t mouse_button_7 = Mouse::Button7;
    inline constexpr uint32_t mouse_button_last =
      Mouse::ButtonLast;
    inline constexpr uint32_t mouse_button_left =
      Mouse::ButtonLeft;
    inline constexpr uint32_t mouse_button_right =
      Mouse::ButtonRight;
    inline constexpr uint32_t mouse_button_middle =
      Mouse::ButtonMiddle;
};