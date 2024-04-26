#include <rama/scripting.hpp>

#include <rama/engine.hpp>
#include <rama/physics3d.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

sol::state lua_state;

bool check_lua_object(sol::object obj) {
    if(!obj.valid()) {
        engine::warning("check_lua_object: lua object is not valid");
        return false;
    }

    return true;
}

bool luakey_pressed(string key) {
    SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());

    if(scancode == SDL_SCANCODE_UNKNOWN) {
        engine::warning("LUA: Engine.key_pressed({}) :: {}", key, SDL_GetError());
    }

    return engine::key_pressed(scancode);
}

bool luakey_released(string key) {
    SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());

    if(scancode == SDL_SCANCODE_UNKNOWN) {
        engine::warning("LUA: Engine.key_released({}) :: {}", key, SDL_GetError());
    }

    return engine::key_released(scancode);
}

bool luakey_held(string key) {
    SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());

    if(scancode == SDL_SCANCODE_UNKNOWN) {
        engine::warning("LUA: Engine.key_held({}) :: {}", key, SDL_GetError());
    }

    return engine::key_held(scancode);
}

sol::table libEngine(sol::this_state state) {
    sol::state_view luaview(state);
    auto module = luaview.create_table();

    module.set_function("DeltaTime", &engine::DeltaTime);

    module.set_function("Info", [](string msg) { engine::info("{}", msg); });
    module.set_function("Warning", [](string msg) { engine::warning("{}", msg); });
    module.set_function("Error", [](string msg) { engine::error("{}", msg); });

    module.set_function("GetGlslVersion", &engine::get_GLSLVersion);

    module["WorldForward"] = engine::WorldForward; 
    module["WorldRight"] = engine::WorldRight; 
    module["WorldUp"] = engine::WorldUp; 

    return module;
}

sol::table libWindow(sol::this_state state) {
    sol::state_view luaview(state);
    auto module = luaview.create_table();

    module.set_function("SetTitle", &engine::set_title);
    module.set_function("SetSize", &engine::set_size);

    module.set_function("Quit", &engine::quit);

    module.set_function("GetPath", &engine::get_path);

    module.set_function("MouseDelta", &engine::mouse_delta);
    module.set_function("MousePos", &engine::mouse_pos);

    module.set_function("KeyPressed", &luakey_pressed);
    module.set_function("KeyReleased", &luakey_released);
    module.set_function("KeyHeld", &luakey_held);

    module.set_function("LockMouse", &engine::lock_mouse);
    module.set_function("UnlockMouse", &engine::unlock_mouse);

    module.set_function("SetClearColor", &engine::set_clear_color);

    return module;
}

sol::table libGFX(sol::this_state state) {
    sol::state_view luaview(state);

    auto module = luaview.create_table();

    sol::constructors<Mesh()> Mesh_ctors;
    module.new_usertype<Mesh>("Mesh",
        Mesh_ctors,
        "load", &Mesh::load,
        "make", &Mesh::make,
        "destroy", &Mesh::destroy,
        "draw", &Mesh::draw
    );

    sol::constructors<Shader()> Shader_ctors;
    module.new_usertype<Shader>("Shader",
        Shader_ctors,
        "load", &Shader::load,
        "make", &Shader::make,
        "destroy", &Shader::destroy,
        "bind", &Shader::bind,
        "uniform", sol::overload(
            sol::resolve<void(string,Mat4)>(&Shader::uniform),
            sol::resolve<void(string,Vec2f)>(&Shader::uniform),
            sol::resolve<void(string,Vec2f)>(&Shader::uniform),
            sol::resolve<void(string,f32)>(&Shader::uniform),
            sol::resolve<void(string,Texture)>(&Shader::uniform)
        )
    );
    
    sol::constructors<Texture()> Texture_ctors;
    module.new_usertype<Texture>("Texture",
        Texture_ctors,
        "make", &Texture::make,
        "destroy", &Texture::destroy,
        "bind", &Texture::bind
    );

    sol::constructors<Sprite()> Sprite_ctors;
    module.new_usertype<Sprite>("Sprite",
        Sprite_ctors,
        "make", &Sprite::make,
        "destroy", &Sprite::destroy,

        "draw", &Sprite::draw,

        "pos", &Sprite::pos,
        "scale", &Sprite::scale
    );

    sol::constructors<FPSCamera()> FPSCamera_ctors;
    module.new_usertype<FPSCamera>("FPSCamera",
        FPSCamera_ctors,
        "GetView", &FPSCamera::GetView,
        "GetPerspective", &FPSCamera::GetPerspective,

        "GetForward", &FPSCamera::GetForward,
        "GetRight", &FPSCamera::GetRight,
        "GetUp", &FPSCamera::GetUp,

        "update", &FPSCamera::update,

        "pos", &FPSCamera::pos,
        "pitch", &FPSCamera::pitch,
        "yaw", &FPSCamera::yaw,
        "roll", &FPSCamera::roll,
        "fov", &FPSCamera::fov,
        "near", &FPSCamera::near,
        "far", &FPSCamera::far
    );

    sol::constructors<Camera2D()> Camera2D_ctors;
    module.new_usertype<Camera2D>("Camera2D",
        Camera2D_ctors,
        "GetView", &Camera2D::GetView,
        "GetPerspective", &Camera2D::GetPerspective,

        "pos", &Camera2D::pos,
        "zoom", &Camera2D::zoom
    );

    sol::constructors<Vec3f(), Vec3f(f32,f32,f32), Vec3f(f32), Vec3f(Vec3f)> Vec3f_ctors;
    module.new_usertype<Vec3f>("Vec3f",
        Vec3f_ctors,
        "x", &Vec3f::x,
        "y", &Vec3f::y,
        "z", &Vec3f::z,

        "length", [](Vec3f& self) { return glm::length(self); },
        "normalize", &engine::safe_normalize,

        sol::meta_function::addition, sol::overload(
            [](Vec3f self, f32 val) { return self + val; },
            [](Vec3f self, Vec3f val) { return self + val; }
        ),
        sol::meta_function::subtraction, sol::overload(
            [](Vec3f self, f32 val) { return self - val; },
            [](Vec3f self, Vec3f val) { return self - val; }
        ),
        sol::meta_function::multiplication, sol::overload(
            [](Vec3f self, f32 val) { return self * val; },
            [](Vec3f self, Vec3f val) { return self * val; }
        ),
        sol::meta_function::division, sol::overload(
            [](Vec3f self, f32 val) { return self / val; },
            [](Vec3f self, Vec3f val) { return self / val; }
        )
    );

    sol::constructors<Vec2f(), Vec2f(f32,f32), Vec2f(f32), Vec2f(Vec2f)> Vec2f_ctors;
    module.new_usertype<Vec2f>("Vec2f",
        Vec2f_ctors,
        "x", &Vec2f::x,
        "y", &Vec2f::y,

        "length", [](Vec2f& self) { return glm::length(self); },
        "normalize", [](Vec2f val) {
            f32 length = glm::length(val);
            return length > 0 ? val / length : Vec2f(0);
        },

        sol::meta_function::addition, sol::overload(
            [](Vec2f self, f32 val) { return self + val; },
            [](Vec2f self, Vec2f val) { return self + val; }
        ),
        sol::meta_function::subtraction, sol::overload(
            [](Vec2f self, f32 val) { return self - val; },
            [](Vec2f self, Vec2f val) { return self - val; }
        ),
        sol::meta_function::multiplication, sol::overload(
            [](Vec2f self, f32 val) { return self * val; },
            [](Vec2f self, Vec2f val) { return self * val; }
        ),
        sol::meta_function::division, sol::overload(
            [](Vec2f self, f32 val) { return self / val; },
            [](Vec2f self, Vec2f val) { return self / val; }
        )
    );

    sol::constructors<Mat4(), Mat4(f32)> Mat4_ctors;
    module.new_usertype<Mat4>("Mat4",
        Mat4_ctors,
        "translate", [](Mat4& matrix, Vec3f val) {
            matrix = glm::translate(matrix, val);
        },
        "rotate", [](Mat4& matrix, f32 angle, Vec3f axis) {
            matrix = glm::rotate(matrix, angle, axis);
        },
        "scale", [](Mat4& matrix, Vec3f val) {
            matrix = glm::scale(matrix, val);
        },
        "perspective", [](f32 fov, f32 aspect_ratio, f32 near, f32 far) {
            return glm::perspective(fov, aspect_ratio, near ,far);
        },
        "orthographic", [](f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
            return glm::ortho(left, right, bottom, top, near, far);
        }
    );

    return module;
}

sol::table libImGui(sol::this_state state) {
    sol::state_view luaview(state);

    auto module = luaview.create_table();

    module.set_function("ShowDemoWindow", []() {
        ImGui::ShowDemoWindow();
    });

    module.set_function("ShowMetricsWindow", []() {
        ImGui::ShowMetricsWindow();
    });

    module.set_function("ShowAboutWindow", []() {
        ImGui::ShowAboutWindow();
    });

    module.set_function("ShowUserGuide", []() {
        ImGui::ShowUserGuide();
    });
    module.set_function("GetVersion", []() -> string {
        return ImGui::GetVersion();
    });

    module.set_function("StyleColorsDark", [](){ ImGui::StyleColorsDark(); });
    module.set_function("StyleColorsLight", [](){ ImGui::StyleColorsLight(); });
    module.set_function("StyleColorsClassic", [](){ ImGui::StyleColorsClassic(); });

    module.set_function("Begin", sol::overload(
        [](string name) {
            ImGui::Begin(name.c_str());
        },
        [](string name, i32 flags) {
            ImGui::Begin(name.c_str(), nullptr, flags);
        }
    ));
    module.set_function("End", &ImGui::End);

    module.set_function("BeginChild", sol::overload(
        [](string id) {
            ImGui::BeginChild(id.c_str());
        },
        [](string id, Vec2f size) {
            ImGui::BeginChild(
                id.c_str(),
                ImVec2(size.x, size.y)
            );
        },
        [](string id, Vec2f size, u32 flags) {
            ImGui::BeginChild(
                id.c_str(),
                ImVec2(size.x, size.y),
                flags
            );
        }
    ));
    module.set_function("EndChild", &ImGui::EndChild);

    //
    // layout functions 
    //
    module.set_function("Seperator", &ImGui::Separator);
    module.set_function("SameLine",
        sol::overload(
            []() { ImGui::SameLine(); },
            [](f32 offset_from_start) { ImGui::SameLine(offset_from_start); },
            [](f32 offset_from_start, f32 spacing) { ImGui::SameLine(offset_from_start, spacing); }
        )
    );
    module.set_function("NewLine", &ImGui::NewLine);
    module.set_function("Spacing", &ImGui::Spacing);

    //
    // Widgets: text
    //
    module.set_function("Text", [](string msg) {
        ImGui::TextUnformatted(msg.c_str());
    });

    //
    // Widgets: main
    //
    module.set_function("Button", sol::overload(
        [](string name) {
            return ImGui::Button(name.c_str());
        },
        [](string name, Vec2f size) {
            return ImGui::Button(name.c_str(), ImVec2(size.x, size.y));
        }
    ));
    module.set_function("SmallButton", [](string label) {
        return ImGui::SmallButton(label.c_str());
    });
    module.set_function("Checkbox", [](string label) {
        bool v;
        ImGui::Checkbox(label.c_str(), &v);
        return v;
    });
    module.set_function("Bullet", &ImGui::Bullet);
    //
    // Widgets: Combo Box (Dropdown)
    //

    //
    // Widgets: Drag Sliders
    //
    module.set_function("DragFloat", sol::overload(
        [](string label, f32 val) {
            ImGui::DragFloat(label.c_str(), &val);
            return val;
        },
        [](string label, f32 val, f32 speed) {
            ImGui::DragFloat(label.c_str(), &val, speed);
            return val;
        },
        [](string label, f32 val, f32 speed, f32 min, f32 max) {
            ImGui::DragFloat(label.c_str(), &val, speed, min, max);
            return val;
        }
    ));
    module.set_function("DragFloat2", sol::overload(
        [](string label, f32 val1, f32 val2) {
            f32 val[2] = { val1, val2 };
            ImGui::DragFloat(label.c_str(), val);
            return std::tuple(val[0], val[1]);
        }
    ));
    module.set_function("DragFloat3", sol::overload(
        [](string label, f32 val1, f32 val2, f32 val3) {
            f32 val[3] = { val1, val2, val3 };
            ImGui::DragFloat3(label.c_str(), val);
            return std::tuple(val[0], val[1], val[2]);
        },
        [](string label, f32 val1, f32 val2, f32 val3, f32 speed) {
            f32 val[3] = { val1, val2, val3 };
            ImGui::DragFloat3(label.c_str(), val, speed);
            return std::tuple(val[0], val[1], val[2]);
        },
        [](string label, f32 val1, f32 val2, f32 val3, f32 speed, f32 min, f32 max) {
            f32 val[3] = { val1, val2, val3 };
            ImGui::DragFloat3(label.c_str(), val, speed, min, max);
            return std::tuple(val[0], val[1], val[2]);
        }
    ));

    module.set_function("BeginMenuBar", &ImGui::BeginMenuBar);
    module.set_function("EndMenuBar", &ImGui::EndMenuBar);

    module.set_function("BeginMenu", sol::overload(
        [](string label) { return ImGui::BeginMenu(label.c_str()); },
        [](string label, bool enabled) {
            return ImGui::BeginMenu(label.c_str(), enabled);
        }
    ));
    module.set_function("EndMenu", &ImGui::EndMenu);

    module.set_function("MenuItem", sol::overload(
        [](string label) {
            return ImGui::MenuItem(label.c_str());
        },
        [](string label, string shortcut) {
            return ImGui::MenuItem(label.c_str(), shortcut.c_str());
        },
        [](string label, string shortcut, bool selected) {
            return ImGui::MenuItem(label.c_str(), shortcut.c_str(), selected);
        }
    ));

    auto moduleflags = module["flags"].get_or_create<sol::table>();
    auto windowflags = moduleflags["window"].get_or_create<sol::table>();

    windowflags["Popup"] = ImGuiWindowFlags_Popup;
    windowflags["NoTitleBar"] = ImGuiWindowFlags_NoTitleBar;
    windowflags["NoResize"] = ImGuiWindowFlags_NoResize;
    windowflags["NoMove"] = ImGuiWindowFlags_NoMove;
    windowflags["NoScrollbar"] = ImGuiWindowFlags_NoScrollbar;
    windowflags["NoScrollWithMouse"] = ImGuiWindowFlags_NoScrollWithMouse;
    windowflags["NoCollapse"] = ImGuiWindowFlags_NoCollapse;
    windowflags["AlwaysAutoResize"] = ImGuiWindowFlags_AlwaysAutoResize;
    windowflags["NoBackground"] = ImGuiWindowFlags_NoBackground;
    windowflags["NoSavedSettings"] = ImGuiWindowFlags_NoSavedSettings;
    windowflags["NoMouseInputs"] = ImGuiWindowFlags_NoMouseInputs;
    windowflags["MenuBar"] = ImGuiWindowFlags_MenuBar;
    windowflags["HorizontalScrollbar"] = ImGuiWindowFlags_HorizontalScrollbar;
    windowflags["NoFocusOnAppearing"] = ImGuiWindowFlags_NoFocusOnAppearing;
    windowflags["NoBringToFrontOnFocus"] = ImGuiWindowFlags_NoBringToFrontOnFocus;
    windowflags["AlwaysVerticalScrollbar"] = ImGuiWindowFlags_AlwaysVerticalScrollbar;
    windowflags["AlwaysHorizontalScrollbar"] = ImGuiWindowFlags_AlwaysHorizontalScrollbar;
    windowflags["NoNavInputs"] = ImGuiWindowFlags_NoNavInputs;
    windowflags["NoNavFocus"] = ImGuiWindowFlags_NoNavFocus;
    windowflags["UnsavedDocument"] = ImGuiWindowFlags_UnsavedDocument;
    windowflags["NoNav"] = ImGuiWindowFlags_NoNav;
    windowflags["NoDecoration"] = ImGuiWindowFlags_NoDecoration;
    windowflags["NoInputs"] = ImGuiWindowFlags_NoInputs;

    module.set_function("CapturedMouse", [](){
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        return io.WantCaptureMouse;
    });
    module.set_function("CapturedKeyboard", [](){
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        return io.WantCaptureKeyboard;
    });

    module.set_function("InputText", sol::overload(
        [](string label, string str) {
            ImGui::InputText(label.c_str(), &str);
            return str;
        },
        [](string label, string str, i32 flags) {
            ImGui::InputText(label.c_str(), &str, flags);
            return str;
        }
    ));
    module.set_function("InputTextMultiline", sol::overload(
        [](string label, string str) {
            ImGui::InputTextMultiline(label.c_str(), &str);
            return str;
        },
        [](string label, string str, Vec2f size) {
            ImGui::InputTextMultiline(
                label.c_str(),
                &str,
                ImVec2(size.x, size.y)
            );
            return str;
        },
        [](string label, string str, Vec2f size, i32 flags) {
            ImGui::InputTextMultiline(
                label.c_str(),
                &str,
                ImVec2(size.x, size.y),
                flags
            );
            return str;
        }
    ));

    return module;
}

namespace scripting {
    sol::state& get_state() {
        return lua_state;
    }

    i32 load(string path) {
        path = engine::get_path(path);

        auto result = lua_state.safe_script_file(path, sol::script_pass_on_error);
        if(!result.valid()) {
            sol::error err = result;
            engine::error("Lua error: {}", err.what());
            return -1;
        }

        return 0;
    }

    sol::safe_function getfunc(string func_name) {
        sol::safe_function func = lua_state[func_name];
        if(!func.valid()) {
            engine::error("Lua: \"{}\" function does not exist", func_name);
        }

        return func;
    }

    void setup() {
        lua_state.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::math,
            sol::lib::coroutine,
            sol::lib::io,
            sol::lib::string,
            sol::lib::table
        );

        lua_state.require("engine", sol::c_call<decltype(&libEngine), &libEngine>, false);
        lua_state.require("window", sol::c_call<decltype(&libWindow), &libWindow>, false);
        lua_state.require("gfx", sol::c_call<decltype(&libGFX), &libGFX>, false);
        lua_state.require("imgui", sol::c_call<decltype(&libImGui), &libImGui>, false);

        physics3d::RegisterLuaModule(lua_state);
    }
}

