{
  description = "Rama engine";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, flake-utils, nixpkgs, ... }@inputs:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          devShells.default = (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
            packages = with pkgs; [
              pkg-config
	            bear
              lldb
              clang-tools
              valgrind
              cmake

              sdl3
              assimp
              freetype
              lua
              sol2
              spdlog
            ];
          });

          packages.default = pkgs.callPackage ./default.nix {};
        }
      );
}
