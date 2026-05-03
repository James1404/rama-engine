{
  lib,
  clangStdenv,
  cmake,
  sdl3,
  assimp,
  freetype,
  lua,
  sol2,
  spdlog
}:

clangStdenv.mkDerivation (finalAttrs: {
  pname = "rama-engine";
  version = "0.01";

  src = ./.;

  nativeBuildInputs = [ cmake ];
  buildInputs = [ sdl3 assimp freetype lua sol2 spdlog ];

  outputs = [];

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
