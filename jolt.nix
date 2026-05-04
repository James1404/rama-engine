{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  pkg-config,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "jolt-physics";
  version = "5.5.0";
  
  src = fetchFromGitHub {
    owner = "jrouwe";
    repo = "JoltPhysics";
    rev = "a80c9dbcb726d7acfa931f30c609615cae15b9e9";
    hash = "sha256-QO6mPYEUPMJSVHj4vhBVzodqUBAp0rYUMrI/s5/S/vk=";
  };

  nativeBuildInputs = [ cmake ];
  buildInputs = [];

  cmakeDir = "../Build";

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF"
    "-DPROFILER_IN_DEBUG_AND_RELEASE=OFF"
    "-DUSE_ASSERTS=OFF"
  ];
  
  meta = with lib; {
    homepage = "https://github.com/jrouwe/JoltPhysics";
    description = ''
    A multi core friendly rigid body physics and collision detection library. Written in C++. Suitable for games and VR applications.
    '';
    licencse = licenses.mit;
    platforms = with platforms; linux;
    maintainers = [];
  };
})
