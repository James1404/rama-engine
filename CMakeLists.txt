cmake_minimum_required(VERSION 3.27.7)

project(rama VERSION 0.01)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rama)

set(rama_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${rama_CURRENT_DIR}/deps/SDL)
add_subdirectory(${rama_CURRENT_DIR}/deps/spdlog)
add_subdirectory(${rama_CURRENT_DIR}/deps/glm)
add_subdirectory(${rama_CURRENT_DIR}/deps/fmt)
add_subdirectory(${rama_CURRENT_DIR}/deps/sol2)

set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS ON)
set(CROSS_PLATFORM_DETERMINISTIC OFF)
set(INTERPROCEDURAL_OPTIMIZATION ON)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(OBJECT_LAYER_BITS 16)

add_subdirectory(${rama_CURRENT_DIR}/deps/JoltPhysics/Build)

add_subdirectory(${rama_CURRENT_DIR}/deps/tinygltf)

set(ASSIMP_WARNINGS_AS_ERRORS false)
add_subdirectory(${rama_CURRENT_DIR}/deps/assimp)

include(FindLua)

add_subdirectory(${rama_CURRENT_DIR}/deps/json)

target_link_libraries(rama PUBLIC
  SDL3::SDL3
  spdlog::spdlog
  glm::glm
  nlohmann_json::nlohmann_json
  fmt::fmt
  sol2
  Jolt
  tinygltf
  assimp
  ${LUA_LIBRARIES}
)
	
file(GLOB imgui_src
    ${rama_CURRENT_DIR}/deps/imgui/*.cpp
    ${rama_CURRENT_DIR}/deps/imgui/backends/imgui_impl_sdl3.cpp
    ${rama_CURRENT_DIR}/deps/imgui/backends/imgui_impl_opengl3.cpp
    ${rama_CURRENT_DIR}/deps/imgui/misc/cpp/imgui_stdlib.cpp
)

file(GLOB rama_src
	${rama_CURRENT_DIR}/src/*.cpp
	${rama_CURRENT_DIR}/src/*.c
)

target_include_directories(rama PUBLIC
    ${rama_CURRENT_DIR}/include
    ${rama_CURRENT_DIR}/deps/glad/include
    ${rama_CURRENT_DIR}/deps/stb
    ${rama_CURRENT_DIR}/deps/imgui
    ${LUA_INCLUDE_DIR}
)

target_sources(rama PRIVATE
    ${rama_src}
    ${rama_CURRENT_DIR}/deps/glad/src/glad.c

    ${imgui_src}
)
