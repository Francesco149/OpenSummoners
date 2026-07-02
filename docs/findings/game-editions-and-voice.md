# Game editions & voice-line map

Living comparison of the SotES retail builds we can inspect + where voice lives.
Voice was the first driver (2026-07-01); the file/engine matrix is meant to grow as
we dig. **Ground truth = read off the retail binaries** (hashes recorded for drift).

## The three installs

| tag | path | release | native engine |
|-----|------|---------|---------------|
| **JP-SE** | `C:\Program Files (x86)\lizsoft\FortuneSummoners` | JP retail-CD *special edition* (Lizsoft, disc dated 2008-2009) | `sotes.exe` 4,214,784 B, **lzsotes-packed** (needs Lizsoft/SPL unpacker, NOT Steamless) |
| **EN-SE** | `C:\Program Files (x86)\Steam\steamapps\common\sotes` | EN Steam *special edition* (newer purchase) | two exes, both SteamStub-v3.1 packed (Steamless-unpackable) |
| **EN-old** | `C:\Program Files (x86)\Steam\steamapps\common\Fortune Summoners` | EN Steam 2012 (Carpe Fulgur) — **our port target** | `sotes.exe` 64,380,928 B, SteamStub |

EN-SE & JP-SE are the **same engine generation** (share byte-identical `sotesd.dll` +
`sotesw.dll`, both carry `sotesx_*` + `sinmode`). EN-old is a **separate, older lineage**
(different `sotesd/w/p`, no `sotesx_*`, no `sinmode`).

## File inventory (asset DLLs = resource-only PE: tiny code, one giant `.rsrc`)

Naming decoded from resource contents (`d`=data/gfx, `w`=wave/BGM, `p`=SE, `x_*`=SE-edition
eXtra, `_en`=English overlay):

| file | JP-SE | EN-SE | EN-old | role (from `.rsrc`) |
|------|-------|-------|--------|---------------------|
| `sotesd.dll`   | 163,987,456 `c5260e5f` | 163,987,456 `c5260e5f` (**==JP**) | 168,083,456 (differs) | main data / sprites + script/scenario |
| `sotesw.dll`   | 81,219,584 `947c9318` | 81,219,584 `947c9318` (**==JP**) | 81,801,216 `d8edc031` (differs) | **BGM** — 47 WMA/ASF streams, resource type `DATA` |
| `sotesp.dll`   | 1,155,072 `a30061…` | 14,188,544 `33a30e…` | 1,155,072 `a93af6…` | **SE** — RIFF/WAVE bank (31 in JP), type `WAVE` (all 3 hashes differ) |
| `sotesx_s.dll` | **253,206,528** | *ABSENT* | *ABSENT* | **VOICE** — see below |
| `sotesx_d.dll` | 17,711,104 | — | — | SE-edition extra data/CG (CD packaging) |
| `sotesx_d2.dll`| — | 87,937,024 | — | SE-edition extra data (Steam packaging) |
| `sotesx_d3.dll`| — | 57,344 | — | tiny extra-data patch |
| `sotesd_en.dll`| — | 9,498,624 | — | **English overlay** — 11 `DATA` blobs (translated text/UI atlas) |
| `sotes.sin` / `_sotes2.ini` | `sinmode=1` | `wait=4` | — | SE flag / config |
| `sotesp.OLD`   | 1,155,072 | — | — | vendor backup of a prior `sotesp` |

## Voice architecture

The engine's asset layer is uniform: `FindResource(<type>, <numeric-id>)` over a DLL.
Audio splits across **three** schemes:

- **Voice** = `sotesx_s.dll`, custom resource type **`WAVE`**: **1,448 leaves**, numeric
  IDs from **1003**, lang **1041 (0x411 = Japanese)**, each a `RIFF….WAVE` clip —
  **PCM, mono, 22050 Hz, 16-bit** (confirmed via `tools/voice_view` (now `tools/res_explorer`); sizes ~47 KB–400 KB
  = ~1–9 s of dialogue; the census's `FF FB` "MP3" hits were coincidental PCM sample
  bytes, NOT MPEG frames). Plus one `DATA` leaf id **1002**, 64 B = manifest/index.
- **BGM** = `sotesw.dll`, type `DATA`: 47 leaves, each an ASF/WMA GUID stream `3026b275…`.
- **SE** = `sotesp.dll`, type `WAVE` (31 small RIFF clips in JP) + a `DATA` index.

So voice is *just another resource bank*; nothing exotic — it hinges entirely on whether
(a) the engine loads `sotesx_s.dll` and (b) the file is present.

## Per-engine references (from unpacked exes)

`grep` of the DLL-name string table + `voice`/`sinmode` (unpacked; JP-retail exe still
lzsotes-packed so not greppable — but EN-SE's `sotes.exe` **is** that same JP-SE engine):

| engine | loads `sotesx_s` (voice)? | loads `sotesd_en`? | Voice-Manager UI | notes |
|--------|:--:|:--:|:--:|-------|
| EN-SE `sotes.exe` (JP build) | **YES** | no | minimal | JP text; refs `sotesx_d2/d3` |
| EN-SE `sotes_en.exe` (EN build) | **NO** | YES | full | EN text; "Deluxe/Original Voices", "Select Combat Voice", "Sync Textbox Advance to Voice", "Setup Voice Manager" |
| EN-old `sotes.exe` | **NO** | n/a (text baked in) | full | refs only `sotesd/w/p`; no `sinmode`/`.sin` |

**Why EN is silent** (user obs: EN has music, dad's opening line unvoiced): the EN-SE
Steam package = the JP-SE engine + an English exe/overlay bolted on, but it **omits the
`sotesx_s.dll` voice payload**. The English build (`sotes_en.exe`, what runs given the
English text) has **no voice-load path at all** — the Voice-Manager "Dialogue Voice"
options are inert. Even the bundled JP `sotes.exe` couldn't voice it: the file is missing.

## The four questions — answers

1. **Where do voice lines live?** JP-SE: `sotesx_s.dll` (1,448 `WAVE` clips). EN-SE:
   *nowhere* — `sotesx_s.dll` not shipped. EN-old: *nowhere* — no voice DLL; engine has a
   combat-voice UI only, no dialogue-voice data/loader.
2. **Do voiceless builds still contain voice data?** **No.** EN-SE retains the voice
   *trigger metadata* (in the byte-identical `sotesd.dll`) + the Voice-Manager UI, but
   **zero voice audio**. EN-old: none. (`sotesd.dll`==JP proves the data DLL carries no
   voice payload — voice is *only* ever in `sotesx_s`.)
3. **Patch EN-SE with JP voice by a file-drop?** *Partly.* Because `sotesd.dll` is
   byte-identical JP↔EN-SE, voice IDs line up with the same dialogue beats.
   - **JP-text + JP-voice: yes, drop-in.** Copy JP `sotesx_s.dll` into the EN-SE dir, launch
     **`sotes.exe`** (the bundled JP engine that references it). ≈ running the JP-SE.
   - **EN-text + JP-voice: NOT by a mere drop.** `sotes_en.exe` has no voice-load path, so
     it ignores the file. Needs a binary patch to `sotes_en.exe` (add the `sotesx_s` load +
     per-line trigger) — **or our port**, which owns the engine.
   - *Confirm empirically:* drop the DLL + listen, or Frida-hook `LoadLibraryA`/
     `FindResourceA` on `sotes.exe` to prove the `WAVE` load. (Not yet run.)
4. **EN-old + JP voice by dropping a DLL?** *No* (as a stock file-drop). The Carpe Fulgur
   exe never loads a `sotesx_s` voice bank (refs only `sotesd/w/p`; no `sinmode`). Dropping
   the DLL is inert. The oft-repeated "just drop in the right DLL" does **not** hold for the
   vanilla exe — it would need an engine/exe swap or patch. **Our port can add EN-text +
   JP-voice** natively (a preservation win retail EN never shipped) → ROADMAP/PORT-DEBT
   candidate.

## Open threads (dig deeper later)

- Empirically verify Q3 (drop `sotesx_s.dll` → `sotes.exe`; Frida `FindResource` proof).
- Codec is settled (PCM / mono / 22050 Hz / 16-bit, via `tools/voice_view` (now `tools/res_explorer`)). Next: map
  clip-ID → dialogue line via the `sotesd.dll` script/scenario table = the basis for
  port voice, and confirm Q3 by ear (drop `sotesx_s.dll`, run `sotes.exe`).
- JP-retail `sotes.exe` is **lzsotes/SPL-packed** — needs the Lizsoft unpacker for the
  authentic 2009 engine; EN-SE's `sotes.exe` is the same generation already unpacked.
- `sotesx_d` (JP CD) vs `sotesx_d2/d3` (Steam) — extra-CG repackaging; compare when relevant.

## Provenance

Measured 2026-07-01. Tools (scratchpad, read-only): `dllprobe.py` (PE sections + audio-
magic census), `rsrc.py` (PE resource-tree walker, mmap). `sha256sum` for identity.
Steamless v3.1 unpacked both EN-SE exes (`sotes.exe`→JP build, `sotes_en.exe`→EN build).
DLL-name/voice references via `grep -a` of the unpacked exes + the EN-old
`vendor/unpacked/sotes.unpacked.exe`. **`tools/voice_view/`** (retired; now `tools/res_explorer/` — `build/res_explorer.exe`,
32-bit Win32 GUI + `--list`/`--dump` CLI) browses/plays/extracts these banks through the
OS loader — the SAME `LoadLibraryEx`/`FindResource` path the engine uses; `--list`
confirmed the 1,448 PCM voice clips, `--dump` extracts byte-perfect WAVs (clip 1003 =
334,360 B, verified).
