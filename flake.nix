{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs =
            with pkgs;
            [
              cmake
              pkg-config
              sdl3
              vulkan-headers
              vulkan-loader
              vulkan-validation-layers
              vulkan-memory-allocator
              shaderc
              spirv-tools
              ktx-tools
              gcc
              gdb
            ];
          shellHook = ''
            export LSAN_OPTIONS=suppressions=$PWD/suppr.txt
          '';
        };
      }
    );
}
