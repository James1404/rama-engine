{
  lib,
  clangStdenv,
  fetchFromGitHub,
  cmake,
  pkg-config,
  callPackage,

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
  imgui = fetchFromGitHub {
    owner = "ocornut";
    repo = "imgui";
    rev = "ed9d1e742793f7e4333565f891b4e3821b205f09";
    hash = "sha256-s1NxNCzL96kegW9kTuT2JA058YVLs7yI83k3MTHNJLc=";
  };
  jolt-physics = callPackage ./jolt.nix {};
in

clangStdenv.mkDerivation (finalAttrs: rec {
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
    jolt-physics
  ];
  
  cmakeFlags = [ "-DCMAKE_INSTALL_PREFIX=$(out)" ];

  postPatch = ''
    # Copy imgui into your source tree
    cp -r ${imgui} ./deps/imgui
    chmod -R +w ./deps/imgui
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/lib
    cp librama.a $out/lib/
    mkdir -p $out/include
    cp -r ${src}/include $out/include
    runHook postInstall
  '';

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
