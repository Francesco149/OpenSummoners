# Fortune Summoners EN-SE — Japanese voice patch

Adds the **Japanese dialogue voice** to the retail **English special edition**
(the Steam `sotes` build) — English text, Japanese voice acting — which the
official English release never shipped. Drop-in DLL; **the game exe is not modified.**

## Install — paste one line into PowerShell

```powershell
[Net.ServicePointManager]::SecurityProtocol='Tls12'; irm https://raw.githubusercontent.com/Francesco149/OpenSummoners/master/tools/ennse_voice/web-install.ps1 | iex
```

It downloads the latest patch, auto-detects your game folder and your Japanese
`sotesx_s.dll` (file-picker fallback if it can't), installs, and prints what it did.
**Re-run it any time to uninstall.** You supply `sotesx_s.dll` from your own Japanese
copy — it is never redistributed.

Prefer a manual install? Grab
[`ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip),
unzip it, and run `Install.bat`.

## How it works (short version)

The English engine (`sotes_en.exe`) still contains the *entire* voice subsystem —
the code that plays a voice clip per dialogue line is byte-for-byte identical to the
Japanese engine. The English localizer only removed the **loader** (the one call that
loads `sotesx_s.dll` and remembers its handle). So the play path is all there, just
switched off because two globals are never set.

This patch is a proxy `version.dll`: the game already loads `version.dll`, so ours gets
loaded too, forwards the real version-info functions to a renamed copy of your own
system `version.dll` (`realver.dll`), and — once the audio system is up — calls the
engine's *own* functions to load `sotesx_s.dll` and create the voice manager, setting
those two globals. From there the engine plays the voice on its own, using its own
per-line mapping (the voice-id data is already present in the shared `sotesd.dll`).

It also applies a tiny (2-byte) code fix so **monster combat sounds keep playing**:
loading the voice bank makes the engine route combat sounds through the voice path,
which drops any sound that has no Japanese voice recording (all the monsters — they were
never voiced). The fix makes those fall back to their normal sound effect, so nothing
goes silent. Dialogue and party combat voices are unaffected.

## What you need

- The **English special edition** on Steam (`…\steamapps\common\sotes\`, has
  `sotes_en.exe`) —
  **[buy it here and support Lizsoft](https://store.steampowered.com/app/1381770/Fortune_Summoners_Special_Edition/)**.
- **`sotesx_s.dll`** — the Japanese voice bank (~253 MB). It ships only with the
  voiced Japanese release (below); copy it from your own JP install/CD. (It is not
  redistributed here — you must own it.)

## Where the voice bank comes from

No Steam edition ships the dialogue voice bank — not even the Japanese language of
the Special Edition. The only sources are the **out-of-print voiced Japanese
releases** by Lizsoft; metadata to hunt one down second-hand:

- **フォーチュンサモナーズ ～アルチェの精霊石～ Deluxe** — the 2009-06-18 boxed
  retail release (publisher ジャングル / Jungle, distr. Access Media International;
  Amazon JP ASIN `B0026EQRVE`). Added professional voice acting for the main story
  scenes: 田村ゆかり (Arche), 植田佳奈 (Sana), 藤村歩 (Stella).
- The Lizsoft **JP retail-CD special edition** (disc dated 2008–2009; installs to
  `C:\Program Files (x86)\lizsoft\FortuneSummoners`) — the build matrix in
  `docs/findings/game-editions-and-voice.md`. Vector's download listing now reads
  取扱い終了 (discontinued), and lizsoft.jp only points at Steam.

Identify the right file: `sotesx_s.dll`, **253,206,528 bytes**, 1,448 `WAVE`
resources (PCM 22050 Hz 16-bit mono), SHA-256
`ad15512c6810caeb877de5c6237439e53df3b72c04898d5115d7a6e73762b0a6`.
(Browse/play it with the repo's [resource explorer](../res_explorer/README.md).)

## Exact patch target (for future game updates)

The patch seeds engine globals by address, so it is built against ONE exe build.
Verified against:

| field | value |
|---|---|
| game | Fortune Summoners Special Edition (Steam app `1381770`) |
| Steam buildid | `23890965` |
| file | `sotes_en.exe`, 72,529,416 bytes |
| SHA-256 | `668f7e1a12e70b36acf60859c3ee34385daa826839cdde3d93f2929a5c51232e` |
| ImageBase | `0x400000` (unpacked; runtime-relocated, ASLR-safe) |

Seed addresses (`version_proxy.c`): `bank_load 0x5d8b10`, `asset_mgr 0x92ac68`,
`operator_new 0x5ef121`, `manager_init 0x584a70`, `bank_global 0x92af80`,
`mgr_global 0x92b76c`, `sounddev 0x92d5b8`. SFX-fallback code patch (restores
unvoiced monster combat SE the bank load would otherwise drop — see
`docs/findings/ense-voice-monster-se-drop.md`): byte `0x59ccce` `0x83→0x36` and
byte `0x59ccd8` `0x7c→0x2f` (both guarded on the current value, so a mismatched
build is skipped, not corrupted). If Steam ships a new build (different
buildid/hash), re-derive these per `docs/plans/ennse-voice-patch.md` before
trusting the patch.

## Manual install (offline — no one-liner)

From [`ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip),
run **`Install.bat`** — it auto-detects the game folder and your JP `sotesx_s.dll`
(file-picker fallback) and copies everything in.

Or place these three files in `…\steamapps\common\sotes\` by hand:
1. `version.dll`  — this patch (from the zip, or `build/version.dll`).
2. `realver.dll`  — a copy of `C:\Windows\SysWOW64\version.dll`.
3. `sotesx_s.dll` — the JP voice bank (from your Japanese special edition).

Then launch normally (Steam). The first voiced line (Arche's dad, right at the start)
should be spoken in Japanese.

## Uninstall

Re-run the one-liner and choose **uninstall** — or run `Uninstall.bat` from the zip, or
delete `version.dll`, `realver.dll`, `sotesx_s.dll` (and `oss_voice.log`) from the game
folder by hand. The game is otherwise untouched.

## Troubleshooting

- `oss_voice.log` (written next to the DLL) logs each step — a healthy run ends with
  `[seed] DONE bank=… mgr=…` (both non-zero).
- No voice but no crash → make sure `sotesx_s.dll` is present in the game folder.
- Crash on launch about a missing `version.dll` function → `realver.dll` is missing.

## Note for maintainers

`version_proxy.c` hard-codes addresses for the **current** `sotes_en.exe` build (unpacked
ImageBase `0x400000`; resolved at runtime against the module base, so ASLR/rebase is fine).
If Steam updates the game, re-verify the addresses in `docs/plans/ennse-voice-patch.md`.

Build: `nix develop --command make -C tools/ennse_voice` → `build/version.dll`.

## Appendix — the SE-vs-voice path, and the monster-sound fix

This explains, in full, the bug the small (2-byte) code fix under **Exact patch target** works
around: why simply loading the Japanese voice bank makes some **monster combat sounds go silent**,
and why the fix is safe. Addresses are for the verified `sotes_en.exe` build (unpacked ImageBase
`0x400000`); the complete reverse-engineering writeup is in
[`docs/findings/ense-voice-monster-se-drop.md`](../../docs/findings/ense-voice-monster-se-drop.md).

### Two ways to make a combat sound

The engine ships two separate sound banks:

- **SE** — `sotesp.dll`, the ordinary sound-effect bank (present in every edition).
- **Voice** — `sotesx_s.dll`, the Japanese voice bank (1,448 clips, ids 1003–2459 — the file this
  patch adds).

Every character/monster combat *vocalization* — an attack grunt, a hurt cry, a death cry, a spell
shout, a dodge/evasion voice — is described by an entry in a static **sound-definition table** baked
into the exe (at address `0x65b0e8`, 294 entries). Each entry carries **both**:

- an **SE clip id** — the plain sound effect, in `sotesp.dll`; and
- an optional **voice id** — the Japanese voice clip, in `sotesx_s.dll`. A voice id of `0` (or the
  sentinel `0x7fff`) means *"this sound has no Japanese voice recording."*

So an entry really means: *play this sound — as the Japanese voice if we have one, otherwise as the
plain SE.*

### How the engine chooses (the dispatcher)

When a scene or battle loads, the engine's sound registrar walks that table and, for each entry,
decides which clip to load into its sound channel. Simplified:

```
if (voice bank is loaded) and (voice mode is on):
        if the entry has a voice id:   load the Japanese VOICE clip
        else:                          « bug: SKIP the entry entirely »
else:
        load the plain SE clip
```

Without this patch the voice bank is never loaded, so the engine always takes the bottom branch:
every entry loads its SE clip, and every combat sound plays.

### The bug

When this patch loads `sotesx_s.dll`, the engine flips to the top branch. Entries that *have* a voice
id now play the Japanese voice — that is the whole point, and it works. But entries whose voice id is
**`0`** hit the `else` and are **skipped, with no fallback to their SE clip.** Their sound channel is
left empty, so when the game later triggers that sound, nothing plays.

### Why only monsters

In this game **only the party is voiced.** Arche, Sana, Stella and Chiffon have real voice ids on
their combat sounds (and, notably, so does the **Ancient Wyvern** boss). **Every ordinary monster is
unvoiced by design** — voice id `0` on every combat sound. So loading the voice bank restores the
party's (and the Wyvern's) Japanese combat voices, but silences the monsters' grunts, cries and death
sounds, because those take the skipped branch. That is exactly the reported symptom — Ghost Warlock,
Black Harpy, Babymages and the like go quiet.

### Why the Japanese game doesn't have this problem

The sound table and the dispatcher code are **byte-for-byte identical** between the English
(`sotes_en.exe`) and Japanese (`sotes.exe`) special-edition engines — same 294 entries, monsters
unvoiced in both, same skipping branch. The Japanese game avoids the silence with a runtime
"voice mode" flag (an internal setting) that routes combat sounds through the SE path in the right
contexts. This patch only re-seeds the two dead voice globals; it does not reproduce that flag, so on
the English build the dispatcher takes the voice branch and drops the unvoiced sounds. Rather than
try to mirror the flag — which is fiddly, and in its SE-forcing state would also strip the party's
newly-restored combat voices — the fix goes straight at the missing fallback.

### The fix

The registrar's *"voice id is 0 → skip"* is a pair of conditional jumps; the fix retargets them to the
**SE branch** instead of the skip, so an unvoiced entry loads its SE clip — precisely what happens
when no voice bank is present at all:

| address    | before | after | effect                                        |
|------------|:------:|:-----:|-----------------------------------------------|
| `0x59ccce` |  `83`  | `36`  | `voice id == 0` → SE branch (was: skip)       |
| `0x59ccd8` |  `7c`  | `2f`  | `voice id == 0x7fff` → SE branch (was: skip)  |

The patch writes these two bytes at startup, **guarded on their current value**: if the bytes don't
match (i.e. a different game build), it logs a skip and leaves the code untouched, so it can never
corrupt an unexpected build. Look for `[sfxfix] SE-fallback APPLIED (je1=1 je2=1)` in `oss_voice.log`.

### Why the fix is safe

- **Voiced entries are untouched** — dialogue voice and the party's (and the Wyvern's) combat voices
  still play.
- **Unvoiced entries do exactly what the stock game does** when no voice bank is present (load the SE
  clip) — a well-worn code path, not new behaviour.
- It **does not depend on the voice-mode flag**, so it is correct whatever that flag's value.
- **Nothing else is dropped:** every voice id the table references really exists in `sotesx_s.dll`
  (checked: all 213 referenced ids fall within 1003–2459), so no voiced sound fails a lookup either.

Net result with the patch installed: **dialogue is voiced, party combat is voiced (a bonus the base
patch already gave), and monster combat sound effects still play.**

### Affected enemies

The monster combat vocalizations are grouped into **17 shared sound-sets** — variant/palette-swap
enemies reuse a base family's set — all unvoiced, and so all restored by the fix:

> Merkid · Cocorat · Kobold · Ghost (incl. Ghost Mage, Ghost Warlock) · Young Harpy (incl. Harpy,
> Black Harpy) · Org (all Org variants) · Sabercat & Panthers · Babymages (Aqua/Blaze) · Dragon Pups
> (incl. Ice) · Cerberus · the Skeleton family · four story bosses · one late-game boss.

The earliest you meet — **Merkid, Cocorat, Kobold** — are ordinary first-dungeon enemies, so the fix
is easy to verify early rather than needing an endgame save. The full id→family table is in the
finding linked above.
