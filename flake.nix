{
  # Yes, this is AI generated, I can't be bothered to actually configure a nix devenv myself for this.
  description = "Flipper Zero development shells (fbt/ufbt) for this repo";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        lib = pkgs.lib;

        armGcc = if pkgs ? arm-none-eabi-gcc then pkgs.arm-none-eabi-gcc else pkgs.gcc-arm-embedded;
        armBinutils = if pkgs ? arm-none-eabi-binutils then pkgs.arm-none-eabi-binutils else null;
        armNewlib = if pkgs ? arm-none-eabi-newlib then pkgs.arm-none-eabi-newlib else null;

        python = pkgs.python3;
        py = python.withPackages (
          ps:
          with ps;
          [
            pip
            setuptools
            wheel
            protobuf
            pyelftools
            pillow
            jinja2
            colorama
            requests
            jsonschema
            pyyaml
            click
          ]
          ++ lib.optionals (ps ? ufbt) [ ps.ufbt ]
        );
      in
      {
        devShells.default = pkgs.mkShell {
          name = "flipper-dev";
          packages = [
            pkgs.git
            pkgs.scons
            pkgs.cmake
            pkgs.ninja
            pkgs.ccache
            pkgs.dfu-util
            pkgs.openocd
            pkgs.usbutils
            pkgs.gnumake
            pkgs.pkg-config
            pkgs.pipx
            armGcc
            py
          ]
          ++ lib.filter (x: x != null) [
            armBinutils
            armNewlib
          ];

          shellHook = ''
            export PATH="$HOME/.local/bin:$PATH"

            # Use a writable directory for fbt's managed toolchain if it wants to download anything
            export FBT_TOOLCHAIN_PATH="$PWD/.fbt-toolchain"
            mkdir -p "$FBT_TOOLCHAIN_PATH"

            # Ensure arm-none-eabi-* tools from nix are on PATH (they already are via packages)
            # If your fbt expects a specific name, this should satisfy it without downloads.

            # Make ufbt available (via nixpkg if present, otherwise pipx)
            if ! command -v ufbt >/dev/null 2>&1; then
              echo "[info] ufbt not found in nixpkgs for this channel; attempting local pipx install..."
              if command -v pipx >/dev/null 2>&1; then
                pipx install --include-deps ufbt >/dev/null 2>&1 || true
              else
                python -m pip install --user --upgrade ufbt || true
              fi
            fi

            echo
            echo "Flipper dev shell ready."
            echo "- ARM toolchain via nix on PATH (arm-none-eabi-gcc)"
            echo "- fbt will use writable: $FBT_TOOLCHAIN_PATH"
            echo "- ufbt available (nixpkgs or pipx)"
            echo
            echo "Examples:"
            echo "  # Firmware-style build (inside firmware repo):"
            echo "  scons -j$(nproc)"
            echo "  # Out-of-tree app build with ufbt:"
            echo "  ufbt update && ufbt build"
            echo
          '';
        };

        devShells.ufbt = pkgs.mkShell {
          name = "flipper-ufbt";
          packages = [
            pkgs.git
            pkgs.pipx
          ];
          shellHook = ''
            export PATH="$HOME/.local/bin:$PATH"
            if ! command -v ufbt >/dev/null 2>&1; then
              pipx install --include-deps ufbt || python -m pip install --user ufbt
            fi
            echo "ufbt shell ready. Try: ufbt update && ufbt build"
          '';
        };
      }
    );
}
