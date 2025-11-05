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
              (imgui.override { IMGUI_BUILD_VULKAN_BINDING = true; IMGUI_BUILD_SDL3_BINDING = true; })
              stb
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
            export VK_LAYER_PATH=${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d
            export LSAN_OPTIONS=suppressions=$PWD/suppr.txt
          '';
        };
      }
    );
}
