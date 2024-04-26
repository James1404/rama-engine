#pragma once
#include <types.hpp>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <glm/ext/matrix_transform.hpp> // translate, rotate, scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> // hash function

#include <fmt/core.h>

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
    f32 zoom;

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
extern void update();
extern void draw();
extern void shutdown();
