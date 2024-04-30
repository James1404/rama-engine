#include <rama/engine.hpp>

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

#include <rama/scripting.hpp>

#include <SDL3/SDL_main.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

namespace { // Local Globals
    string glslVersion = "#version 460\n";

    Vec3f clearcolor(0.0, 0.0, 0.0);

    bool running = true;

    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;

    bool window_focused = false;

    string title = "No Title Specified";
    u32 sdl_width = 800, sdl_height = 600;

    f32 game_width = 800, game_height = 600;

    i32 keyboardsize = 0;
    const u8* keyboard = nullptr;
    u8* prevkeyboard = nullptr;

    Vec2f mousepos, prevmousepos, mousedelta;
    u32 prevmousestate = 0, mousestate = 0;

    Mat4 perspective, view;

    bool mouselock = false;

    bool keyboard_block = false, mouse_block = false;

    f32 dt = 0.0;

    string exe_path = "";

    bool opengl_shader_error(string idname, u32 id) {
        i32 success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);

        if(!success) {
            char log[512];
            glGetShaderInfoLog(id, 512, nullptr, log);

            engine::error("{} Shader Error: {}", idname, log);
        }

        return success;
    }

    bool opengl_program_error(u32 id) {
        i32 success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);

        if(!success) {
            char log[512];
            glGetProgramInfoLog(id, 512, nullptr, log);

            engine::error("Program Error: {}", log);
        }

        return success;
    }
}

LineInstancing LineInstancing::make() {
    string vtx_shader = R"(
        layout(std460, binding = 0) buffer TVertex
        {
            vec4 vertex[];
        };

        uniform mat4 perspective;
        uniform mat4 view;

        uniform float thickness;
        uniform vec2 resolution;

        void main() {
            int line_i = gl_VertexID / 6;
            int tri_i  = gl_VertexID % 6;

            vec4 va[4];
            for (int i=0; i<4; ++i)
            {
                va[i] = perspective * view * vertex[line_i+i];
                va[i].xyz /= va[i].w;
                va[i].xy = (va[i].xy + 1.0) * 0.5 * resolution;
            }

            vec2 v_line  = normalize(va[2].xy - va[1].xy);
            vec2 nv_line = vec2(-v_line.y, v_line.x);

            vec4 pos;
            if (tri_i == 0 || tri_i == 1 || tri_i == 3)
            {
                vec2 v_pred  = normalize(va[1].xy - va[0].xy);
                vec2 v_miter = normalize(nv_line + vec2(-v_pred.y, v_pred.x));

                pos = va[1];
                pos.xy += v_miter * thickness * (tri_i == 1 ? -0.5 : 0.5) / dot(v_miter, nv_line);
            }
            else
            {
                vec2 v_succ  = normalize(va[3].xy - va[2].xy);
                vec2 v_miter = normalize(nv_line + vec2(-v_succ.y, v_succ.x));

                pos = va[2];
                pos.xy += v_miter * thickness * (tri_i == 5 ? 0.5 : -0.5) / dot(v_miter, nv_line);
            }

            pos.xy = pos.xy / resolution * 2.0 - 1.0;
            pos.xyz *= pos.w;
            gl_Position = pos;
        }
    )";

    string frg_shader = R"(
        out vec4 fragColor;

        void main() {
            fragColor = vec4(0.2, 1.0, 0.3, 1.0);
        }
    )";

    LineInstancing result;

    result.shader = Shader::make_with_version(vtx_shader, frg_shader);

    glGenVertexArrays(1, &result.vao);
    glBindVertexArray(result.vao);
    glBindVertexArray(0);

    return result;
}

void LineInstancing::destroy() {
    glDeleteVertexArrays(1, &vao);

    shader.destroy();
}

void LineInstancing::add(Vec3f p1, Vec3f p2, Vec3f colour, f32 thickness) {
}

void LineInstancing::draw(Mat4 perspective, Mat4 view) {

}

Texture Texture::make(string path) {
    path = engine::get_path(path);

    Texture result;

    i32 w,h,ncomp;
    result.data = stbi_load(path.c_str(), &w, &h, &ncomp, 0);

    u32 format = GL_RG;
    switch(ncomp) {
        case 2:
            format = GL_RG;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
    }

    if(!result.data) {
        engine::error("Failed to load texture data: \"{}\"", path);
        return result;
    }

    glGenTextures(1, &result.GLid);
    glBindTexture(GL_TEXTURE_2D, result.GLid);

    glTexParameteri(result.GLid, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(result.GLid, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(result.GLid, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(result.GLid, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0 , format, GL_UNSIGNED_BYTE, result.data);

    result.path = path;
    return result;
}

void Texture::destroy() {
    glDeleteTextures(1, &GLid);
    stbi_image_free(data);
}

void Texture::bind(i32 unit) {
    unit = GL_TEXTURE0 + unit;
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, GLid);
}

Shader Shader::load(string path) {
    path = engine::get_path(path);
    string vShaderCode, fShaderCode;
    std::ifstream ifs(path);
    if (ifs.is_open())
    {
        enum class line_type_
        {
            none = 0,
            vertex,
            fragment,
            version
        } line_type = line_type_::none;

        assert(fShaderCode.empty());
        assert(vShaderCode.empty());

        string line;
        while (std::getline(ifs, line))
        {
            if (line == "@vertex")
            {
                line_type = line_type_::vertex;
                continue;
            }
            else if (line == "@fragment")
            {
                line_type = line_type_::fragment;
                continue;
            }

            if (line_type == line_type_::vertex)
            {
                vShaderCode += line;
                vShaderCode += "\n";
            }
            else if (line_type == line_type_::fragment)
            {
                fShaderCode += line;
                fShaderCode += "\n";
            }
        }

        ifs.close();
    }
    else
    {
        std::cout << "Failed to open shader file: " << path << '\n';
    }

    return Shader::make_with_version(vShaderCode, fShaderCode);
}

Shader Shader::make(string vertex_src, string fragment_src) {
    const char* vsrc = vertex_src.c_str();
    const char* fsrc = fragment_src.c_str();

    u32 vertex = glCreateShader(GL_VERTEX_SHADER);
    u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex, 1, &vsrc, NULL);
    glShaderSource(fragment, 1, &fsrc, NULL);

    glCompileShader(vertex);
    opengl_shader_error("Vertex", vertex);
    glCompileShader(fragment);
    opengl_shader_error("Fragment", fragment);

    u32 program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);
    opengl_program_error(program);

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    Shader result;
    result.program = program;
    return result;
}

Shader Shader::make_with_version(string vertex_src, string fragment_src) {
    string version = engine::get_GLSLVersion();
    vertex_src.insert(0, version);
    fragment_src.insert(0, version);
    return Shader::make(vertex_src, fragment_src);
}

void Shader::destroy() {
    glDeleteProgram(program);
}

void Shader::bind() {
    glUseProgram(program);
}

void Shader::uniform(string name, Mat4 val) {
    i32 loc = glGetUniformLocation(program, name.c_str());
    glUniformMatrix4fv(loc, 1, false, glm::value_ptr(val));
}

void Shader::uniform(string name, Vec3f val) {
    i32 loc = glGetUniformLocation(program, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(val));
}

void Shader::uniform(string name, Vec2f val) {
    i32 loc = glGetUniformLocation(program, name.c_str());
    glUniform2fv(loc, 1, glm::value_ptr(val));
}

void Shader::uniform(string name, f32 val) {
    i32 loc = glGetUniformLocation(program, name.c_str());
    glUniform1f(loc, val);
}

void Shader::uniform(string name, Texture val) {
    i32 loc = glGetUniformLocation(program, name.c_str());
    glUniform1i(loc, val.unit);
}

Mesh Mesh::load(string path) {
    path = engine::get_path(path);
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace       |
        aiProcess_GenSmoothNormals       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_FlipUVs                |
        aiProcess_OptimizeMeshes         |
        aiProcess_SortByPType
    );

    if(!scene) {
        engine::error("Assimp error: {}", importer.GetErrorString());
    }

    ArrayList<Vec3f> vertices;
    ArrayList<Vec2f> uvs;
    ArrayList<Vec3f> normals;
    ArrayList<Vec3f> tangents;
    ArrayList<Vec3f> bitangents;
    ArrayList<u32> indices;

    std::function<void(aiMesh* mesh)> process_mesh;
    process_mesh = [&](aiMesh* mesh) {
        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            vertices.push_back(Vec3f(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            ));

            normals.push_back(Vec3f(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            ));

            if(mesh->HasTangentsAndBitangents()) {
                tangents.push_back(Vec3f(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                ));

                bitangents.push_back(Vec3f(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                ));
            }

            if (mesh->mTextureCoords[0]) {
                uvs.push_back(Vec2f(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                ));
            }
            else {
                uvs.push_back(Vec2f(0));
            }
        }

        for(u32 i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(u32 j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
    };

    std::function<void(aiNode* node)> process_node;
    process_node = [&](aiNode* node) -> void {
        for(u32 i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            process_mesh(mesh);
        }

        for(u32 i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i]);
        }
    };

    process_node(scene->mRootNode);

    return Mesh::make(
        vertices,
        uvs,
        normals,
        tangents,
        bitangents,
        indices
    );
}

Mesh Mesh::make(
    ArrayList<Vec3f> vertices,
    ArrayList<Vec2f> uvs,
    ArrayList<Vec3f> normals,
    ArrayList<Vec3f> tangents,
    ArrayList<Vec3f> bitangents,
    ArrayList<u32> indices)
{
    Mesh result;
    result.vertices = vertices;
    result.uvs = uvs;
    result.normals = normals;
    result.tangents = tangents;
    result.bitangents = bitangents;
    result.indices = indices;

    usize size = 0;

    size += sizeof(result.vertices[0]) * result.vertices.size();
    size += sizeof(result.uvs[0]) * result.uvs.size();
    size += sizeof(result.normals[0]) * result.normals.size();
    size += sizeof(result.tangents[0]) * result.tangents.size();
    size += sizeof(result.bitangents[0]) * result.bitangents.size();

    glGenVertexArrays(1, &result.vao);
    glBindVertexArray(result.vao);

    glGenBuffers(1, &result.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);

    usize i = 0;

    glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(result.vertices[0]) * result.vertices.size(), result.vertices.data());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)i);
    i += sizeof(result.vertices[0]) * result.vertices.size();

    glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(result.uvs[0]) * result.uvs.size(), result.uvs.data());
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(result.uvs[0]), (void*)i);
    i += sizeof(result.uvs[0]) * result.uvs.size();

    glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(result.normals[0]) * result.normals.size(), result.normals.data());
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(result.normals[0]), (void*)i);
    i += sizeof(result.normals[0]) * result.normals.size();

    glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(result.tangents[0]) * result.tangents.size(), result.tangents.data());
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(result.tangents[0]), (void*)i);
    i += sizeof(result.tangents[0]) * result.tangents.size();

    glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(result.bitangents[0]) * result.bitangents.size(), result.bitangents.data());
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(result.bitangents[0]), (void*)i);
    i += sizeof(result.bitangents[0]) * result.bitangents.size();

    glGenBuffers(1, &result.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(result.indices[0])*result.indices.size(), result.indices.data(), GL_STATIC_DRAW);

    return result;
}

void Mesh::destroy() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

void Mesh::draw() {
    glBindVertexArray(vao);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

Sprite Sprite::make(string path) {
    Sprite result;

    using json = nlohmann::json;

	std::ifstream ifs(path);
	json data = json::parse(ifs);

	for(auto& [key, value] : data.items()) {
        using Frame = SpriteSheetMetadata::Frame;
        SpriteSheetMetadata metadata;
		metadata.name = key;
        metadata.fps = value["fps"];

		for(auto& elem : value["frames"]) {
			metadata.frames.push_back(Frame {
				elem["x"], elem["y"], elem["w"], elem["h"]
			});
		}

        result.animations.emplace(key, metadata);
	}
    return result;
}

void Sprite::destroy() {

}

void Sprite::draw() {
}

Mat4 FPSCamera::GetView() {
    Mat4 matrix(1);
    matrix *= rotation;
    matrix *= transform;
    return matrix;
}

Mat4 FPSCamera::GetPerspective() {
    return glm::perspective(glm::radians(fov), (f32)game_width / (f32)game_height, near, far);
}

Vec3f FPSCamera::GetForward() {
    return forward;
}

Vec3f FPSCamera::GetRight() {
    return right;
}

Vec3f FPSCamera::GetUp() {
    return up;
}

void FPSCamera::update() {
    Mat4 pitchMat = glm::rotate(Mat4(1), glm::radians(pitch), engine::WorldRight);
    Mat4 yawMat = glm::rotate(Mat4(1), glm::radians(yaw  ), engine::WorldUp);
    Mat4 rollMat = glm::rotate(Mat4(1), glm::radians(roll ), engine::WorldForward);

    rotation = pitchMat * yawMat * rollMat;

    transform = Mat4(1);
    transform = glm::translate(transform, -pos);

    forward = glm::normalize(glm::inverse(rotation) * Vec4f(engine::WorldForward, 1));
    right = glm::normalize(glm::cross(forward, engine::WorldUp));
    up = glm::normalize(glm::cross(forward, right));
}

Mat4 Camera2D::GetView() {
    Mat4 matrix(1.0);

    matrix = glm::translate(matrix, Vec3f(pos, 0));
    matrix = glm::scale(matrix, Vec3f(zoom));

    return matrix;
}

Mat4 Camera2D::GetPerspective() {
    return glm::ortho(0.0f, (f32)game_width, 0.0f, (f32)game_height, 0.0f, 1000.0f);
}

Framebuffer Framebuffer::make(bool depth_test = false) {
    Framebuffer result;

    result.depth_test = depth_test;

    glGenFramebuffers(1, &result.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);

    glGenTextures(1, &result.albedo);
    glBindTexture(GL_TEXTURE_2D, result.albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game_width, game_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.albedo, 0);

    glGenRenderbuffers(1, &result.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, result.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, game_width,game_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, result.rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        engine::error("glGenFramebuffers failed");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return result;
}

void Framebuffer::destroy() {
    glDeleteFramebuffers(1, &fbo);
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::clear(f32 r, f32 b, f32 g) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(depth_test) {
        glEnable(GL_DEPTH_TEST);
    }
    else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Framebuffer::UpdateSize(f32 w, f32 h) {
    w = std::max(1.0f, w);
    h = std::max(1.0f, h);
    bind();

    glBindTexture(GL_TEXTURE_2D, albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    unbind();
}

namespace {
    LineInstancing lineinstancing;
}

namespace engine {
    f32 DeltaTime() {
        return dt;
    }

    Vec3f safe_normalize(Vec3f val) {
        f32 length = glm::length(val);
        return length > 0 ? val / length : Vec3f(0);
    }


	void set_title(string _title) {
		title = _title;
		if (window) {
			SDL_SetWindowTitle(window, title.c_str());
		}
	}

	void set_size(i32 w, i32 h) {
		sdl_width = w;
		sdl_height = h;
		if (window) {
			SDL_SetWindowSize(window, sdl_width, sdl_height);
		}
	}

	void quit() {
		running = false;
	}

    bool key_pressed(SDL_Scancode key) {
        if(keyboard_block) return false;
        return keyboard[key] && !prevkeyboard[key];
    }

    bool key_released(SDL_Scancode key) {
        if(keyboard_block) return false;
        return !keyboard[key] && prevkeyboard[key];
    }

    bool key_held(SDL_Scancode key) {
        if(keyboard_block) return false;
        return keyboard[key];
    }

    Vec2f mouse_delta() {
        return mousedelta;
    }

    Vec2f mouse_pos() {
        return mousepos;
    }

    bool mouse_pressed(MouseButton button) {
        if(mouse_block) return false;

        switch(button) {
            case MouseButton::left: {
                return (mousestate & SDL_BUTTON_LMASK) != 0 &&
                    !(prevmousestate & SDL_BUTTON_LMASK);
            } break;
            case MouseButton::right: {
                return (mousestate & SDL_BUTTON_RMASK) != 0 &&
                    !(prevmousestate & SDL_BUTTON_RMASK);
            } break;
            case MouseButton::middle: {
                return (mousestate & SDL_BUTTON_MMASK) != 0 &&
                    !(prevmousestate & SDL_BUTTON_MMASK);
            } break;
        }

        return false;
    }

    bool mouse_released(MouseButton button) {
        if(mouse_block) return false;

        switch(button) {
            case MouseButton::left: {
                return !(mousestate & SDL_BUTTON_LMASK) &&
                    (prevmousestate & SDL_BUTTON_LMASK) != 0;
            } break;
            case MouseButton::right: {
                return !(mousestate & SDL_BUTTON_RMASK) &&
                    (prevmousestate & SDL_BUTTON_RMASK) != 0;
            } break;
            case MouseButton::middle: {
                return !(mousestate & SDL_BUTTON_MMASK) &&
                    (prevmousestate & SDL_BUTTON_MMASK) != 0;
            } break;
        }

        return false;
    }

    bool mouse_held(MouseButton button) {
        if(mouse_block) return false;

        switch(button) {
            case MouseButton::left: {
                return (mousestate & SDL_BUTTON_LMASK) != 0;
            } break;
            case MouseButton::right: {
                return (mousestate & SDL_BUTTON_RMASK) != 0;
            } break;
            case MouseButton::middle: {
                return (mousestate & SDL_BUTTON_MMASK) != 0;
            } break;
        }

        return false;
    }
    
    string get_path(string relative) {
        return fmt::format("{}/{}", exe_path, relative);
    }

    void lock_mouse() {
        mouselock = true;
    }

    void unlock_mouse() {
        mouselock = false;
    }

    SDL_GLContext get_gpu_context() {
        return context;
    }

    SDL_Window* get_window() {
        return window;
    }

    string get_GLSLVersion() {
        return glslVersion;
    }

    Vec2f get_game_size() {
        return Vec2f(game_width, game_height);
    }

    void set_framebuffer(Framebuffer& frame) {
    }

    void set_clear_color(f32 x, f32 y, f32 z) {
        clearcolor = Vec3f(x,y,z);
    }
} // namespace engine

int main(int argc, char** argv) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL_Error: " << SDL_GetError() << "\n";
		return 1;
	}

    char* exe_path_ptr = SDL_GetBasePath();
    exe_path = string(exe_path_ptr);
    delete exe_path_ptr;

	window = SDL_CreateWindow(title.c_str(), sdl_width, sdl_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	if (!window) {
		std::cout << "SDL_Error: " << SDL_GetError() << "\n";
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    auto context = SDL_GL_CreateContext(window);
	if(!context) {
        std::cout<< "SDL_Error: " << "Failed to initialize OpenGL context" << std::endl;
        return 1;
	}
	else SDL_GL_MakeCurrent(window, context);
	
	if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        std::cout << "GLAD_Error" << "Failed to load OpenGL" << std::endl;
        return 1;
    }

    glEnable(GL_DEPTH_TEST);  
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glFrontFace(GL_CW);

    keyboard = SDL_GetKeyboardState(&keyboardsize);
    prevkeyboard = new u8[keyboardsize];
    memset(prevkeyboard, 0, keyboardsize);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForOpenGL(
        engine::get_window(),
        engine::get_gpu_context()
    );
    string ver = engine::get_GLSLVersion();
    ImGui_ImplOpenGL3_Init(ver.c_str());

    dt = 0;
    std::chrono::high_resolution_clock clock;
    auto current = clock.now(), previous = clock.now();

    auto framebuffer = Framebuffer::make(true);

    scripting::setup();

	if(i32 e = init(); e < 0) {
		return e;
	}

	while(running) {
        mousedelta = Vec2f(0);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    window_focused = true;
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    window_focused = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    sdl_width = event.window.data1;
                    sdl_height = event.window.data2;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    prevmousestate = mousestate;

                    mousepos = Vec2f(
                        event.motion.x,
                        event.motion.y
                    );

                    mousedelta = Vec2f(
                        event.motion.xrel,
                        event.motion.yrel
                    );

                    prevmousestate = event.motion.state;

                    break;
				default:
					break;
			}
		}

        if(window_focused) {
            if(SDL_SetRelativeMouseMode(mouselock) < 0) {
                engine::error("SDL_SetRelativeMouseMode: {}", SDL_GetError());
            }
        }

		previous = current;
		current = clock.now();

		dt = std::chrono::duration<float>(current - previous).count();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        ImGui::Begin("Game View");

        mouse_block = !ImGui::IsWindowFocused();
        keyboard_block = !ImGui::IsWindowFocused();

        game_width = ImGui::GetContentRegionAvail().x;
        game_height = ImGui::GetContentRegionAvail().y;

        framebuffer.UpdateSize(game_width, game_height);
        glViewport(0, 0, game_width, game_height);

        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImGui::GetWindowDrawList()->AddImage(
            (void*)(isize)framebuffer.albedo, 
            ImVec2(pos.x, pos.y), 
            ImVec2(pos.x + game_width, pos.y + game_height), 
            ImVec2(0, 1), 
            ImVec2(1, 0)
        );

        ImGui::End();

        framebuffer.bind();
        framebuffer.clear(clearcolor.x, clearcolor.y, clearcolor.z);
        update();
        draw();

        ImGui::Render();

        framebuffer.unbind();

		glClearColor(clearcolor.x, clearcolor.y, clearcolor.z, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

        prevkeyboard = (u8*)memcpy(prevkeyboard, keyboard, keyboardsize*sizeof(*keyboard));
	}

	shutdown(); // shutdown game

    framebuffer.destroy();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    delete[] prevkeyboard;

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
