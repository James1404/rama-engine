cmake_minimum_required(VERSION 3.27.7)

project(rama VERSION 0.01)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rama)

set(rama_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig QUIET)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(assimp REQUIRED CONFIG)
find_package(Freetype)
find_package(sol2 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(TinyGLTF)
find_package(nlohmann_json REQUIRED)
find_package(Jolt REQUIRED)

pkg_check_modules(STB REQUIRED stb)

# add_subdirectory(${rama_CURRENT_DIR}/deps/spdlog)
# add_subdirectory(${rama_CURRENT_DIR}/deps/glm)
# add_subdirectory(${rama_CURRENT_DIR}/deps/fmt)
# add_subdirectory(${rama_CURRENT_DIR}/deps/sol2)

set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS ON)
set(CROSS_PLATFORM_DETERMINISTIC OFF)
set(INTERPROCEDURAL_OPTIMIZATION ON)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(OBJECT_LAYER_BITS 16)

include(FindLua)

target_link_libraries(rama
PUBLIC
  SDL3::SDL3
  spdlog::spdlog
  glm::glm
  nlohmann_json::nlohmann_json
  fmt::fmt
  Jolt
  assimp
  ${LUA_LIBRARIES}
  Freetype::Freetype
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
    $<INSTALL_INTERFACE:include>

    $<BUILD_INTERFACE:${rama_CURRENT_DIR}/include>
    $<BUILD_INTERFACE:${rama_CURRENT_DIR}/deps/glad/include>
    $<BUILD_INTERFACE:${rama_CURRENT_DIR}/deps/imgui>
    $<BUILD_INTERFACE:${STB_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${JoltPhysics_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${LUA_INCLUDE_DIR}>

    $<BUILD_INTERFACE:${sol2_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${TinyGLTF_INCLUDE_DIRS}>
)

target_sources(rama PRIVATE
    ${rama_src}
    ${rama_CURRENT_DIR}/deps/glad/src/glad.c

    ${imgui_src}
)

include(CMakePackageConfigHelpers)

# Generate the config file
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/rama-engineConfig.cmake
  INSTALL_DESTINATION lib/cmake/rama-engine
)

# Generate the version file
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/rama-engineConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY deps/glad/include/ DESTINATION include)
install(DIRECTORY deps/imgui/ DESTINATION include)

install(TARGETS rama
  EXPORT rama-engineTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(EXPORT rama-engineTargets
  FILE rama-engineTargets.cmake
  NAMESPACE rama-engine::
  DESTINATION lib/cmake/rama-engine
)

# Install the config and version files
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/rama-engineConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/rama-engineConfigVersion.cmake
  DESTINATION lib/cmake/rama-engine
)

