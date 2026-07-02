# res_explorer — the SotES resource explorer

Native Windows viewer/exporter for **every resource type** in Fortune Summoners'
game files (Dear ImGui + DX11, mingw-cross, 32-bit to match the game DLLs).
Successor of the old `tools/voice_view` audio lister — same OS resource path the
engine itself uses (`LoadLibraryEx` + `FindResource`), now with real decoders.

    nix develop --command make -C tools/res_explorer    # -> build/res_explorer.exe
    (run on Windows)  build\res_explorer.exe             # auto-loads the game install

Ships no game data: it reads the user's own installed files at runtime.
Don't own the game? **[Buy the Special Edition on Steam](https://store.steampowered.com/app/1381770/Fortune_Summoners_Special_Edition/)**
and support Lizsoft.

## What it understands

| kind      | container                         | payload                                   | preview                                         | export |
|-----------|-----------------------------------|-------------------------------------------|-------------------------------------------------|--------|
| Sprite    | `sotesd.dll` / exe `DATA`         | Lizsoft compressed 8/24bpp (`0x2711` sig) | pixels + palette panel, colorkey, frame grid    | PNG, per-frame PNGs |
| Image     | `DATA` (raw-path / plain BMP)     | 8bpp+pal / 24bpp BGR, magenta key         | same image viewer                               | PNG    |
| Map       | exe `DATA` (`MSD_SOTES_MAPDATA`)  | tilemap cells + object layers             | tile-id schematic, plane filter, object overlay | JSON   |
| SFX/Voice | `sotesd`/`sotesp`/`sotesx_s` `WAVE`| RIFF PCM                                  | waveform, seekable playback (waveOut)           | WAV    |
| BGM       | `sotesw.dll` `DATA`               | ASF/WMA stream                            | MCI playback (temp-file staged)                 | WMA    |
| Strings   | `RT_STRING`                       | UTF-16 tables                             | decoded text                                    | TXT    |
| anything  | any                               | —                                         | hex + info tabs always available                | BIN    |

Decoders are the **engine's own ported code** (`src/bitmap_session.c` — raw +
self-rebasing compressed header, embedded palettes; `src/map_data.c` — the
FUN_00587970 parse), so what you see is what the engine decodes, byte for byte.
The tool supplies its own `bs_load_pe_resource` with the 1041-language fallback
(the game DLLs are single-language Japanese datafiles).

## UX map

- **Toolbar** — `Load game install` (auto-detects Steam/lizsoft dirs, loads every
  `sotes*.exe|dll`), `Open DLL…`, live filter (id/info text), kind + module
  filters, `Export all filtered…` (bulk, picks per-kind format + manifest).
- **Left** — one unified sortable table across all loaded modules
  (Module | Type | ID | Kind | Size | Info), colored kind badges, clipper-backed
  (100k+ rows stay 60 fps). `↑/↓` moves selection.
- **Right** — `Preview | Hex | Info` tabs + a per-kind export bar.
  - Images: wheel zoom (fit/1:1..16x), pan, colorkey toggle (magenta, quirk #47),
    V-flip (sheets are bottom-up DIBs at every depth — flipped upright by
    default, toggle shows raw memory order), frame-grid overlay
    with cell W/H (dims aren't in the 8bpp container — quirk of the format; the
    compressed header DOES carry sheet dims), hovered-pixel readout, palette panel.
  - Audio: peak waveform, click-to-seek, space = play/pause, loop, volume.
  - Maps: golden-ratio tile-id coloring, per-plane toggles, object markers colored
    by type range (50k EFFECT / 60k STRUCTURE / 70k CHARACTER / 80k DEVICE),
    hover = cell/object detail. Badge shows `consumed == size` (parses exactly).
- **Fonts** — Segoe UI merged with MS Gothic (Japanese glyphs) when present.

## Headless CLI (kept from voice_view, scriptable via WSLInterop)

    res_explorer.exe --list <dll> <out.txt>            enumerate types/names
    res_explorer.exe --dump <dll> <type> <id> <out>    extract one raw resource
    res_explorer.exe --export <dll> <type> <id> <out>  extract in the kind-appropriate
                                                       format (png/json/txt/wav/raw)
    res_explorer.exe --shot <out.png> [<dll-or-dir> [TYPE:ID]]   render + screenshot (CI/docs)

## Legal / CI

Pure code; the no-assets gate (`tools/ci/no_proprietary_bytes.py`) scans the
built exe. Detection constants are deliberately short prefixes (never the full
16-byte ASF GUID, never a laid-out RIFF header) so format *parsing* can't be
mistaken for embedded *assets*.
