{
  lib,
  clangStdenv,
  fetchFromGitHub,
  cmake,
  pkg-config,

  sdl3,
  assimp,
  freetype,
  lua,
  sol2,
  spdlog,
  glm,
  fmt,
  imgui,
  tinygltf,
  nlohmann_json
}:

let
  # JoltPhysics is fetched at build time via FetchContent in deps/joltc/CMakeLists.txt.
  # Pre-fetch it here and pass via FETCHCONTENT_SOURCE_DIR_JOLTPHYSICS to avoid network access.
  joltphysics-src = fetchFromGitHub {
    owner = "jrouwe";
    repo = "JoltPhysics";
    rev = "a80c9dbcb726d7acfa931f30c609615cae15b9e9";
    hash = "sha256-QO6mPYEUPMJSVHj4vhBVzodqUBAp0rYUMrI/s5/S/vk=";
  };
  imgui = fetchFromGitHub {
    owner = "ocornut";
    repo = "imgui";
    rev = "ed9d1e742793f7e4333565f891b4e3821b205f09";
    hash = "sha256-s1NxNCzL96kegW9kTuT2JA058YVLs7yI83k3MTHNJLc=";
  };
in

clangStdenv.mkDerivation (finalAttrs: {
  pname = "rama-engine";
  version = "0.01";

  src = ./.;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [
    sdl3
    assimp
    freetype
    lua
    sol2
    spdlog
    glm
    fmt
    imgui
    tinygltf
    nlohmann_json
  ];

  postPatch = ''
    # Copy imgui into your source tree
    cp -r ${imgui} ./deps/imgui
    chmod -R +w ./deps/imgui
  '';

  cmakeFlags = [
    "-DFETCHCONTENT_SOURCE_DIR_JOLTPHYSICS=${joltphysics-src}"
  ];

  meta = with lib; {
    homepage = "https://github.com/James1404/rama-engine";
    description = ''
      A small library i built for making simple games / programs.
    '';
    licencse = licenses.mit;
    platforms = with platforms; linux;
    maintainers = [];   
  };
})
