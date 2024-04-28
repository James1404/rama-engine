#pragma once
#include <rama/types.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// For emscripten, instead of using glad we use its built-in support for OpenGL:
#include <GL/gl.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <glm/ext/matrix_transform.hpp> // translate, rotate, scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> // hash function

#include <fmt/core.h>

#include <imgui.h>

#include <nlohmann/json.hpp>

class Texture {
private:
    string path;
    u8* data;
    u32 GLid;
public:
    i32 unit = 0;

    static Texture make(string path);
    void destroy();
    void bind(i32 unit);
};

class Shader {
private:
    string path;
    u32 program;
public:
    static Shader load(string path);
    static Shader make(string vertex_src, string fragment_src);
    static Shader make_with_version(string vertex_src, string fragment_src);

    void destroy();
    void bind();
    
    void uniform(string name, Mat4 val);
    void uniform(string name, Vec3f val);
    void uniform(string name, Vec2f val);
    void uniform(string name, f32 val);
    void uniform(string name, Texture val);
};

class Mesh {
private:
    ArrayList<Vec3f> vertices;
    ArrayList<Vec2f> uvs;
    ArrayList<Vec3f> normals;
    ArrayList<Vec3f> tangents;
    ArrayList<Vec3f> bitangents;
    ArrayList<u32> indices;

    u32 vao, vbo, ibo;
public:
    static Mesh load(string path);
    static Mesh make(
        ArrayList<Vec3f> vertices,
        ArrayList<Vec2f> uvs,
        ArrayList<Vec3f> normals,
        ArrayList<Vec3f> tangents,
        ArrayList<Vec3f> bitangents,
        ArrayList<u32> indices
    );
    void destroy();

    void draw();
};

struct SpriteSheetMetadata {
    struct Frame {
        i32 x,y,w,h;
    };
    string name;
    ArrayList<Frame> frames;
    f32 fps = 1.0f / 6.0f;
};

class Sprite {
private:
    u32 vbo;
    Map<string, SpriteSheetMetadata> animations;
public:
    Vec2f pos, scale;

    Sprite make(string path);
    void destroy();

    void draw();
};

class Camera {
public:
    virtual ~Camera() {}

    virtual Mat4 GetView() = 0;
    virtual Mat4 GetPerspective() = 0;

    virtual Vec3f GetForward() { return Vec3f(0); }
    virtual Vec3f GetRight() { return Vec3f(0); }
    virtual Vec3f GetUp() { return Vec3f(0); }

    virtual void update() {}
};

class FPSCamera : public Camera {
private:
    Mat4 rotation, transform; 

    Vec3f forward, right, up;
public:
    Vec3f pos;
    f32 pitch, yaw, roll;
    f32 fov = 70.0f;
    f32 near = 0.01, far = 1000.0;

    Mat4 GetView();
    Mat4 GetPerspective();

    Vec3f GetForward();
    Vec3f GetRight();
    Vec3f GetUp();

    void update();
};

class Camera2D : public Camera {
public:
    Vec2f pos;
    f32 zoom = 1.0f;

    Mat4 GetView();
    Mat4 GetPerspective();
};

class Framebuffer {
public:
    u32 fbo, rbo, albedo;
    static Framebuffer make();
    void destroy();

    void bind();
    void unbind();

    void clear(f32 r, f32 b, f32 g);

    void UpdateSize(f32 width, f32 height);
};

class LineInstancing {
public:
    struct Line {
        Vec3f p1, p2, colour;
        f32 thickness;
    };
private:
    string vtx_shader = R"(
        uniform mat4 perspective;
        uniform mat4 view;

        layout(location = 0) in vec3 a_vertex;
        layout(location = 1) in vec3 a_p1;
        layout(location = 2) in vec3 a_p2;
        layout(location = 3) in vec3 a_colour;
        layout(location = 4) in f32 a_thickness;

        out vec3 o_colour;

        void main() {
            gl_Position = perspective * view * a_model * vec4(a_vertex, 1.0);

            o_colour = a_colour;
        }
    )";

    string frg_shader = R"(
        in vec3 color;

        out vec4 frag;

        void main() {
            frag = vec4(colour, 1.0);
        }
    )";

    u32 vao, vbo, ebo, instance;
    Shader shader;

    ArrayList<Vec3f> vertices = {
        Vec3f(0.0,  0.5, -0.5),
        Vec3f( 0.5, -0.5, -0.5),
        Vec3f(-0.5, -0.5, -0.5),
    };

    ArrayList<u32> indices = {
        0, 1, 2,
    };

    static_assert(std::is_standard_layout<Line>::value, "Line must have standard layout");
    static_assert(std::is_trivially_copyable<Line>::value, "Line must be trivially copyable");

    Stack<Line> stack;
public:
    static LineInstancing make();
    void destroy();
    void add(Vec3f p1, Vec3f p2, Vec3f colour, f32 thickness);
    void draw(Mat4 perspective, Mat4 view);
};

namespace engine {
    constexpr auto WorldForward = Vec3f(0, 0, -1);
    constexpr auto WorldUp = Vec3f(0, 1, 0);
    constexpr auto WorldRight = Vec3f(1, 0, 0);

    f32 DeltaTime();

    Vec3f safe_normalize(Vec3f val);

    void set_title(string _title);
    void set_size(i32 w, i32 h);
	void quit();

    bool key_pressed(SDL_Scancode key);
    bool key_released(SDL_Scancode key);
    bool key_held(SDL_Scancode key);

    Vec2f mouse_delta();
    Vec2f mouse_pos();

    enum class MouseButton {
        left, right, middle
    };

    bool mouse_pressed(MouseButton button);
    bool mouse_released(MouseButton button);
    bool mouse_held(MouseButton button);

    string get_path(string relative);

    void lock_mouse();
    void unlock_mouse();

    SDL_GLContext get_gpu_context();
    SDL_Window* get_window();

    string get_GLSLVersion();

    Vec2f get_game_size();

    void draw_line(Vec3f p1, Vec3f p2, Vec3f colour, f32 thickness);

    void set_camera(Camera* camera);

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }

    void set_clear_color(f32 x, f32 y, f32 z);
} // namespace engine

//
// the game will implement these
//
extern i32 init();
extern void shutdown();
extern void update();
extern void draw();
