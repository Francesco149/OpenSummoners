{
  description = "OpenSummoners — open-source reimplementation of Fortune Summoners (educational RE / preservation)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # 32-bit mingw cross-compiler — sotes.exe is 32-bit Win32 PE,
        # Steam DRM wrapper (.bind section), 2012-era MSVC.  Companion DLLs
        # (sotesd.dll / sotesp.dll / sotesw.dll) and our drop-in all target
        # i686 to match the original's process bitness.
        mingw32 = pkgs.pkgsCross.mingw32.buildPackages;

        # Dear ImGui sources for the resource explorer (tools/res_explorer) and
        # the trainer UI.  We compile the .cpp directly into the mingw32 Windows
        # PE (DX11 backend) — ImGui is source-vendored, not a lib, so we only
        # need the checkout.  IMGUI_DIR is exported into the dev shell.
        imguiSrc = pkgs.imgui.src;

        # Python environment for tooling.  Same shape as the OpenMare /
        # openrecet siblings so authoring habits transfer:
        #   - pillow / numpy / scikit-image / opencv4 for frame work
        #   - construct for binary-format parsers
        #   - rich for nicer CLI output
        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          pillow
          numpy
          scikit-image
          opencv4
          pytest
          pytest-xdist
          pyyaml
          construct
          rich
        ]);

      in {
        devShells.default = pkgs.mkShell {
          name = "opensummoners-dev";

          packages = with pkgs; [
            # ── reverse engineering ───────────────────────────────────
            # (the decompile is IDA 9.4 on the Windows box: tools/ida/)
            radare2
            rizin
            cutter
            imhex
            hexyl
            bvi
            file
            binutils          # nm, objdump, strings
            icoutils          # wrestool/icotool — extract resources from PE

            # ── build toolchain (32-bit Win32 target) ─────────────────
            mingw32.gcc       # i686-w64-mingw32-gcc — produces Win32 PE
            mingw32.binutils
            gnumake
            cmake
            ninja
            pkg-config

            # ── media ─────────────────────────────────────────────────
            # (rhost runs on the Windows box over ssh, or locally under the
            # host's wine + Xvfb: tools/rhost/README.md)
            ffmpeg            # frame extraction from any video captures

            # ── image/asset processing ────────────────────────────────
            imagemagick
            pngquant
            optipng

            # ── python ────────────────────────────────────────────────
            pythonEnv

            # ── docs / reporting ──────────────────────────────────────
            pandoc

            # ── general dev ────────────────────────────────────────────
            git
            git-lfs
            jq
            ripgrep
            fd
            bat
            tree
          ];

          shellHook = ''
            export OPENSUMMONERS_ROOT=$PWD
            export OPENSUMMONERS_GAME_DIR="/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners"
            export OPENSUMMONERS_STEAMLESS_DIR="/mnt/c/Users/headpats/Documents/_devtools/Steamless.v3.1.0.5.-.by.atom0s"

            # Propagate OPENSUMMONERS_GAME_DIR across the WSL→Windows boundary
            # so .exes launched via WSLInterop can see it.  The /p flag asks
            # WSL to auto-translate /mnt/X/... to X:\... when crossing into
            # Windows, so the .exe gets a native Windows path with no
            # client-side translation needed.
            export WSLENV="''${WSLENV:+$WSLENV:}OPENSUMMONERS_GAME_DIR/p"

            # mingw cross-compiler convenience aliases.
            export MINGW_CC=i686-w64-mingw32-gcc
            export MINGW_AR=i686-w64-mingw32-ar
            export MINGW_STRIP=i686-w64-mingw32-strip

            # Dear ImGui source checkout (res_explorer / the trainer UI —
            # DX11 backend compiled into the Windows PE).
            export IMGUI_DIR=${imguiSrc}

            echo "opensummoners dev shell ready"
            echo "  game dir:   $OPENSUMMONERS_GAME_DIR"
            echo "  steamless:  $OPENSUMMONERS_STEAMLESS_DIR"
            echo "  mingw cc:   $(command -v $MINGW_CC || echo '(missing)')"
            echo ""
            echo "Bootstrap: ./tools/setup.sh"
          '';
        };

        # Lean CI shell — just the cross toolchain + native gcc + make + python
        # (stdlib) + zip.  Used by .github/workflows/* so cold builds stay fast:
        # the default shell pulls cutter/imhex/opencv/… (a big closure).
        # No game assets are ever needed — the port/patch/tools are pure code.
        devShells.ci = pkgs.mkShell {
          name = "opensummoners-ci";
          packages = [
            mingw32.gcc       # i686-w64-mingw32-gcc/g++ — port + patch + tools
            mingw32.binutils
            pkgs.gcc          # native host gcc — the ASan unit suite (tests/)
            pkgs.gnumake
            pkgs.python3      # stdlib only — tools/ci/no_proprietary_bytes.py
            pkgs.coreutils
            pkgs.zip          # package the voice-patch release zip
          ];
          # res_explorer (ImGui/DX11) compiles the flake-provided ImGui sources
          shellHook = ''
            export IMGUI_DIR=${imguiSrc}
          '';
        };
      });
}
