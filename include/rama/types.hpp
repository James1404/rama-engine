#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <stack>

#include <glm/vec2.hpp> // glm::vec3
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::Mat4

template<typename T>
using ArrayList = std::vector<T>;

template<typename T, std::size_t size>
using Array= std::array<T, size>;

template<typename Key, typename T>
using Map = std::map<Key, T>;

template<typename Key, typename T>
using UnorderedMap = std::unordered_map<Key, T>;

template<typename T>
using Stack = std::stack<T>;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = uintptr_t;
using isize = intptr_t;

using f32 = float;
using f64 = double;

using string = std::string;

using Vec2d = glm::dvec2;
using Vec2f = glm::vec2;
using Vec2i = glm::ivec2;
using Vec2u = glm::uvec2;

using Vec3d = glm::dvec3;
using Vec3f =  glm::vec3;
using Vec3i = glm::ivec3;
using Vec3u = glm::uvec3;

using Vec4d = glm::dvec4;
using Vec4f =  glm::vec4;
using Vec4i = glm::ivec4;
using Vec4u = glm::uvec4;

using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

#define TODO(...)\
    do {\
        engine::warning("__FILE__ :: TODO at line __LINE__: {}", __VA_ARGS__);\
    } while(0)

