module;

#include <cstdint>

export module events:keyboard;


export {
    enum Key : uint32_t {
        // From glfw3.h
        Space = 32,
        Apostrophe = 39, /* ' */
        Comma = 44,      /* , */
        Minus = 45,      /* - */
        Period = 46,     /* . */
        Slash = 47,      /* / */

        D0 = 48, /* 0 */
        D1 = 49, /* 1 */
        D2 = 50, /* 2 */
        D3 = 51, /* 3 */
        D4 = 52, /* 4 */
        D5 = 53, /* 5 */
        D6 = 54, /* 6 */
        D7 = 55, /* 7 */
        D8 = 56, /* 8 */
        D9 = 57, /* 9 */

        Semicolon = 59, /* ; */
        Equal = 61,     /* = */

        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,

        LeftBracket = 91,  /* [ */
        Backslash = 92,    /* \ */
        RightBracket = 93, /* ] */
        GraveAccent = 96,  /* ` */

        World1 = 161, /* non-US #1 */
        World2 = 162, /* non-US #2 */

        /* Function keys */
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,

        /* Keypad */
        KP0 = 320,
        KP1 = 321,
        KP2 = 322,
        KP3 = 323,
        KP4 = 324,
        KP5 = 325,
        KP6 = 326,
        KP7 = 327,
        KP8 = 328,
        KP9 = 329,
        KPDecimal = 330,
        KPDivide = 331,
        KPMultiply = 332,
        KPSubtract = 333,
        KPAdd = 334,
        KPEnter = 335,
        KPEqual = 336,

        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        Leftsuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        Rightsuper = 347,
        Menu = 348
    };

    // From glfw3.h
    inline constexpr uint32_t key_space = Key::Space;
    inline constexpr uint32_t key_apostrophe =
      Key::Apostrophe;                                  /* ' */
    inline constexpr uint32_t key_comma = Key::Comma;   /* , */
    inline constexpr uint32_t key_minus = Key::Minus;   /* - */
    inline constexpr uint32_t key_period = Key::Period; /* . */
    inline constexpr uint32_t key_slash = Key::Slash;   /* / */
    inline constexpr uint32_t key_0 = Key::D0;
    inline constexpr uint32_t key_1 = Key::D1;
    inline constexpr uint32_t key_2 = Key::D2;
    inline constexpr uint32_t key_3 = Key::D3;
    inline constexpr uint32_t key_4 = Key::D4;
    inline constexpr uint32_t key_5 = Key::D5;
    inline constexpr uint32_t key_6 = Key::D6;
    inline constexpr uint32_t key_7 = Key::D7;
    inline constexpr uint32_t key_8 = Key::D8;
    inline constexpr uint32_t key_9 = Key::D9;
    inline constexpr uint32_t key_semicolon =
      Key::Semicolon;                                 /* ; */
    inline constexpr uint32_t key_equal = Key::Equal; /* = */
    inline constexpr uint32_t key_a = Key::A;
    inline constexpr uint32_t key_b = Key::B;
    inline constexpr uint32_t key_c = Key::C;
    inline constexpr uint32_t key_d = Key::D;
    inline constexpr uint32_t key_e = Key::E;
    inline constexpr uint32_t key_f = Key::F;
    inline constexpr uint32_t key_g = Key::G;
    inline constexpr uint32_t key_h = Key::H;
    inline constexpr uint32_t key_i = Key::I;
    inline constexpr uint32_t key_j = Key::J;
    inline constexpr uint32_t key_k = Key::K;
    inline constexpr uint32_t key_l = Key::L;
    inline constexpr uint32_t key_m = Key::M;
    inline constexpr uint32_t key_n = Key::N;
    inline constexpr uint32_t key_o = Key::O;
    inline constexpr uint32_t key_p = Key::P;
    inline constexpr uint32_t key_q = Key::Q;
    inline constexpr uint32_t key_r = Key::R;
    inline constexpr uint32_t key_s = Key::S;
    // inline constexpr uint32_t key_t = Key::T;
    inline constexpr uint32_t key_u = Key::U;
    inline constexpr uint32_t key_v = Key::V;
    inline constexpr uint32_t key_w = Key::W;
    inline constexpr uint32_t key_x = Key::X;
    inline constexpr uint32_t key_y = Key::Y;
    inline constexpr uint32_t key_z = Key::Z;
    inline constexpr uint32_t key_left_bracket =
      Key::LeftBracket; /* [ */
    inline constexpr uint32_t key_backslash =
      Key::Backslash; /* \ */
    inline constexpr uint32_t key_right_bracket =
      Key::RightBracket; /* ] */
    inline constexpr uint32_t key_gravity_accent =
      Key::GraveAccent; /* ` */
    inline constexpr uint32_t key_world_1 =
      Key::World1; /* non-US #1 */
    inline constexpr uint32_t key_world_2 =
      Key::World2; /* non-US #2 */

    /* Function keys */
    inline constexpr uint32_t key_escape = Key::Escape;
    inline constexpr uint32_t key_enter = Key::Enter;
    inline constexpr uint32_t key_tab = Key::Tab;
    inline constexpr uint32_t key_backspace = Key::Backspace;
    inline constexpr uint32_t key_insert = Key::Insert;
    inline constexpr uint32_t key_delete = Key::Delete;
    inline constexpr uint32_t key_right = Key::Right;
    inline constexpr uint32_t key_left = Key::Left;
    inline constexpr uint32_t key_down = Key::Down;
    inline constexpr uint32_t key_up = Key::Up;
    inline constexpr uint32_t key_page_up = Key::PageUp;
    inline constexpr uint32_t key_page_down = Key::PageDown;
    inline constexpr uint32_t key_home = Key::Home;
    inline constexpr uint32_t key_end = Key::End;
    inline constexpr uint32_t key_caps_lock = Key::CapsLock;
    inline constexpr uint32_t key_scroll_lock = Key::ScrollLock;
    inline constexpr uint32_t key_num_lock = Key::NumLock;
    inline constexpr uint32_t key_print_screen =
      Key::PrintScreen;
    inline constexpr uint32_t key_pause = Key::Pause;
    inline constexpr uint32_t key_f1 = Key::F1;
    inline constexpr uint32_t key_f2 = Key::F2;
    inline constexpr uint32_t key_f3 = Key::F3;
    inline constexpr uint32_t key_f4 = Key::F4;
    inline constexpr uint32_t key_f5 = Key::F5;
    inline constexpr uint32_t key_f6 = Key::F6;
    inline constexpr uint32_t key_f7 = Key::F7;
    inline constexpr uint32_t key_f8 = Key::F8;
    inline constexpr uint32_t key_f9 = Key::F9;
    inline constexpr uint32_t key_f10 = Key::F10;
    inline constexpr uint32_t key_f11 = Key::F11;
    inline constexpr uint32_t key_f12 = Key::F12;
    inline constexpr uint32_t key_f13 = Key::F13;
    inline constexpr uint32_t key_f14 = Key::F14;
    inline constexpr uint32_t key_f15 = Key::F15;
    inline constexpr uint32_t key_f16 = Key::F16;
    inline constexpr uint32_t key_f17 = Key::F17;
    inline constexpr uint32_t key_f18 = Key::F18;
    inline constexpr uint32_t key_f19 = Key::F19;
    inline constexpr uint32_t key_f20 = Key::F20;
    inline constexpr uint32_t key_f21 = Key::F21;
    inline constexpr uint32_t key_f22 = Key::F22;
    inline constexpr uint32_t key_f23 = Key::F23;
    inline constexpr uint32_t key_f24 = Key::F24;
    inline constexpr uint32_t key_f25 = Key::F25;

    /* Keypad */
    inline constexpr uint32_t key_kp_0 = Key::KP0;
    inline constexpr uint32_t key_kp_1 = Key::KP1;
    inline constexpr uint32_t key_kp_2 = Key::KP2;
    inline constexpr uint32_t key_kp_3 = Key::KP3;
    inline constexpr uint32_t key_kp_4 = Key::KP4;
    inline constexpr uint32_t key_kp_5 = Key::KP5;
    inline constexpr uint32_t key_kp_6 = Key::KP6;
    inline constexpr uint32_t key_kp_7 = Key::KP7;
    inline constexpr uint32_t key_kp_8 = Key::KP8;
    inline constexpr uint32_t key_kp_9 = Key::KP9;
    inline constexpr uint32_t key_kp_decimal = Key::KPDecimal;
    inline constexpr uint32_t key_kp_divide = Key::KPDivide;
    inline constexpr uint32_t key_kp_multiply = Key::KPMultiply;
    inline constexpr uint32_t key_kp_subtract = Key::KPSubtract;
    inline constexpr uint32_t key_kp_add = Key::KPAdd;
    inline constexpr uint32_t key_kp_enter = Key::KPEnter;
    inline constexpr uint32_t key_kp_equal = Key::KPEqual;

    inline constexpr uint32_t key_left_shift = Key::LeftShift;
    inline constexpr uint32_t key_left_control =
      Key::LeftControl;
    inline constexpr uint32_t key_left_alt = Key::LeftAlt;
    inline constexpr uint32_t key_left_super = Key::Leftsuper;
    inline constexpr uint32_t key_right_shift = Key::RightShift;
    inline constexpr uint32_t key_right_control =
      Key::RightControl;
    inline constexpr uint32_t key_right_alt = Key::RightAlt;
    inline constexpr uint32_t key_right_super = Key::Rightsuper;
    inline constexpr uint32_t key_menu = Key::Menu;
};