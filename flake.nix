{
  description = "FortDB Devshell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            stdenv.cc
            libgcc
            gnumake
            valgrind
            gdb

            pkg-config

            man-pages
            man-pages-posix

            glm
          ];

          shellHook = ''
            echo "FortDB ready to develop."
          '';
        };
      }
    );
}

