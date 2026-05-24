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

        # Python environment for tooling.  Same shape as the OpenMare /
        # openrecet siblings so authoring habits transfer:
        #   - pillow / numpy / scikit-image / opencv4 for frame work
        #   - frida 17.x for Windows-side instrumentation
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
          frida-python
        ]);

      in {
        devShells.default = pkgs.mkShell {
          name = "opensummoners-dev";

          packages = with pkgs; [
            # ── reverse engineering ───────────────────────────────────
            ghidra
            radare2
            rizin
            cutter
            imhex
            hexyl
            bvi
            file
            binutils          # nm, objdump, strings
            icoutils          # wrestool/icotool — extract resources from PE

            # ── dynamic analysis / instrumentation ────────────────────
            # frida-tools is the CLI.  The Windows-side frida-server.exe
            # runs on the host (binds 0.0.0.0:27042); we talk to it via the
            # WSL NAT.  Same instance serves all three sibling projects.
            # No wine: sotes.exe runs through WSLInterop directly on the
            # Windows host so DirectDraw / DirectSound / DirectInput all
            # work with zero setup.
            frida-tools

            # ── build toolchain (32-bit Win32 target) ─────────────────
            mingw32.gcc       # i686-w64-mingw32-gcc — produces Win32 PE
            mingw32.binutils
            gnumake
            cmake
            ninja
            pkg-config

            # ── headless testing ──────────────────────────────────────
            # The exe runs via WSLInterop on the host Windows desktop.
            # The Frida agent applies hide-window from inside the process;
            # no Xvfb / scrot / wine needed.
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

            # Shared Frida instance.  cutestation.soy is the Windows host's
            # LAN-resolvable name; frida-server.exe binds 0.0.0.0:27042 there
            # and serves all three sibling RE projects.  We do NOT default to
            # 127.0.0.1 because WSL2's NAT layer doesn't loop back to the
            # Windows host's localhost binding — the LAN hostname is the
            # reliable path.  Override OPENSUMMONERS_FRIDA_REMOTE explicitly
            # to use a different host/port.
            export OPENSUMMONERS_FRIDA_REMOTE="''${OPENSUMMONERS_FRIDA_REMOTE:-cutestation.soy:27042}"

            echo "opensummoners dev shell ready"
            echo "  game dir:   $OPENSUMMONERS_GAME_DIR"
            echo "  steamless:  $OPENSUMMONERS_STEAMLESS_DIR"
            echo "  mingw cc:   $(command -v $MINGW_CC || echo '(missing)')"
            echo "  frida:      $OPENSUMMONERS_FRIDA_REMOTE (Windows host)"
            echo "  exe runs via WSLInterop (no wine)"
            echo ""
            echo "Bootstrap: ./tools/setup.sh"
          '';
        };

        # Package output: the opensummoners.exe binary cross-compiled with
        # mingw32.  Stub — wired up properly once src/ has a buildable program.
        packages.opensummoners = pkgs.stdenv.mkDerivation {
          pname = "opensummoners";
          version = "0.0.0-dev";
          src = ./src;

          nativeBuildInputs = [ mingw32.gcc mingw32.binutils pkgs.gnumake ];

          buildPhase = ''
            echo "opensummoners build stub — src/ has only a skeleton WinMain"
            make
            mkdir -p $out/bin
            cp ../build/opensummoners.exe $out/bin/opensummoners.exe || true
          '';

          installPhase = "true";

          meta = with pkgs.lib; {
            description = "Open-source drop-in for Fortune Summoners' sotes.exe (educational/preservation)";
            license = licenses.mit;
            platforms = [ "x86_64-linux" ];
          };
        };
      });
}
