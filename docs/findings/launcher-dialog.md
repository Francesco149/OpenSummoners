# Launcher dialog — DLGPROC reverse

The startup launcher window discovered in engine-quirks.md §3 is driven by a
DLGPROC at **0x004013c0** invoked from **`FUN_005b0d70`** via
`DialogBoxParamA(hInst, 0x2711, NULL, dlgProc=0x004013c0, 0)`.

Ghidra's auto-analysis missed 0x004013c0 (it's only referenced as a function
pointer argument), so the disassembly below comes from radare2 with manual
`af`.  Sibling helper **`FUN_00401730`** (107 bytes) is part of the same
component — Ghidra picked that one up because it's reached by a plain call.

## Disassembly map

DLGPROC signature: standard Win32 `INT_PTR CALLBACK DlgProc(HWND hDlg,
UINT uMsg, WPARAM wParam, LPARAM lParam)` — confirmed by the `ret 0x10`
stdcall epilogue (16 bytes = 4 args).

### Dispatch (0x4013c0–0x4013da)

```asm
mov  eax, [esp+8]      ; uMsg
sub  eax, 0x110        ; 0x110 = WM_INITDIALOG
je   0x401545          ; → WM_INITDIALOG handler (populate controls)
dec  eax
jne  0x401722          ; default: return 0
                       ; fall through → WM_COMMAND handler
```

Only `WM_INITDIALOG` and `WM_COMMAND` are handled; every other message
returns 0 (the DefDialogProc skip is correct for a DLGPROC).

### WM_COMMAND switch (0x4013da–0x40151c)

LOWORD(wParam) = control ID.  Five terminal cases:

| ID     | dec   | meaning              | side effect                        |
|--------|-------|----------------------|------------------------------------|
| `2`    | IDCANCEL | implicit Cancel  | `DAT_008a9a40 = 0`, fall through to "scrape" |
| `0x2713` | 10003 | **Launch** button | `DAT_008a9a40 = 1`, fall through to "scrape" |
| `0x2715` | 10005 | (unmapped Cancel?) | `DAT_008a9a40 = 0`, fall through to "scrape" |
| `0x2724` | 10020 | Windowed radio   | jump to 0x40152d → `FUN_00401730(hDlg)` |
| `0x2726` | 10022 | Fullscreen radio | jump to 0x40152d → `FUN_00401730(hDlg)` |
| `0x2729` | 10025 | Zoom-1280 radio  | jump to 0x40152d → `FUN_00401730(hDlg)` |

`DAT_008a9a40` is the **"Launch was clicked"** flag.  Anything else returns
without persisting state.

### Scrape state into globals (0x401424–0x40151a)

When Launch (or one of the Cancel-equivalents) fires, the proc reads every
group's current selection via `IsDlgButtonChecked` and writes a single
enum byte into each of four global `WORD`s:

| global         | controls polled                                | value if checked |
|----------------|------------------------------------------------|------------------|
| `DAT_008a9b48` (screen mode) | 0x2724 (10020 Windowed)             | 3 |
|                              | 0x2726 (10022 Fullscreen)           | 4 |
|                              | 0x2729 (10025 Zoom 1280)            | 5 |
| `DAT_008a9b4a` (VRAM)        | 0x271f (10015 Minimal)              | 3 |
|                              | 0x2720 (10016 Normal)               | 4 |
|                              | 0x2721 (10017 Use All)              | 5 |
| `DAT_008a9b4c` (quality)     | 0x2716 (10006 High)                 | 3 |
|                              | 0x2717 (10007 Medium)               | 4 |
|                              | 0x2718 (10008 Low)                  | 5 |
| `DAT_008a9b4e` (disable sound) | 0x2728 (10024)                    | 4 if checked, 3 otherwise |

Then `EndDialog(hDlg)` returns to `FUN_005b0d70`, which returns
`*DAT_008a9a40` to its caller (0 if cancelled, 1 if Launch).

> ⚠ The radio enum uses **3/4/5** — not 0/1/2 — for the three positions.
> This is the byte that lands in `config.dat` (see below).

### WM_INITDIALOG (0x401545–0x401721) — populate from saved state

1. `SetWindowTextA(hDlg, &DAT_008a9a44)` — title comes from a global
   filled earlier (likely the "Fortune Summoners Ver1.2…" string).
2. `SetFocus(GetDlgItem(hDlg, 0x2711=10001))` — default focus.
3. For each saved field, `CheckRadioButton` to restore the right radio:
   - `DAT_008a9b4a` ∈ {3,4,5} → CheckRadioButton(hDlg, 0x271F, 0x2721, ID)
   - `DAT_008a9b48` ∈ {3,4,5} → CheckRadioButton(hDlg, 0x2724, 0x2729, ID)
   - `DAT_008a9b4c` ∈ {3,4,5} → CheckRadioButton(hDlg, 0x2716, 0x2718, ID)
   - `DAT_008a9b4e` ∈ {3,4} → `CheckDlgButton(hDlg, 0x2728, on/off)`
4. **`ShowWindow(GetDlgItem(hDlg, 0x272A=10026), 0)`** — *unconditionally
   hide the Zoom 1920x1440 radio*.  It exists in the dialog resource but
   the user never sees it.  See **quirks §5** below.
5. Call `FUN_00401730(hDlg)` to grey out the VRAM group if Fullscreen is
   the current selection.
6. Permanently disable controls 0x271C, 0x271D, 0x271E (IDs 10012/13/14)
   — vestigial controls in the resource that the engine no longer honours.
7. `SetForegroundWindow(hDlg)`.

### `FUN_00401730` — VRAM-group dependency on Fullscreen

```c
void EnableVRAMGroup(HWND hDlg) {
    BOOL fullscreen = IsDlgButtonChecked(hDlg, 0x2726) == 1;
    UINT en = fullscreen ? 1 : 0;
    EnableWindow(GetDlgItem(hDlg, 0x271f), en);   // Minimal
    EnableWindow(GetDlgItem(hDlg, 0x2720), en);   // Normal
    EnableWindow(GetDlgItem(hDlg, 0x2721), en);   // Use All
    EnableWindow(GetDlgItem(hDlg, 0x2722), en);   // VRAM-group label
    EnableWindow(GetDlgItem(hDlg, 0x2727), en);   // (warning/extra label)
}
```

So VRAM-use is only configurable in Fullscreen mode.  Called both from
WM_INITDIALOG and from the WM_COMMAND screen-mode-radio path.

## Where do the saved values live?

The engine ships **`user/config.dat`** (840 bytes, XOR-obfuscated) which
holds these launcher settings plus the rest of the user prefs (keybindings,
volumes, etc.).  Quick hex peek of a real file:

```
000000 10 00 00 00 11 27 00 00 34 03 00 00 db 56 00 00   ← plaintext 16-byte header
000010 34 51 bc 3d 8c 88 88 88 f5 05 63 88 8a e3 67 88   ← XOR-obfuscated payload
...                                                       (key includes 0x88; runs of
                                                           '88 88 88 88' decode to 0)
```

Likely structure (TBD when we wire the extractor):
- Header: `[u32 hdr_size=16][u32 version=10001][u32 data_size=820][u32 checksum]`
- Body: 824 bytes of XOR-obfuscated payload containing the 4 launcher
  enums + the full settings struct.

The 4 launcher fields sit at known offsets *somewhere* in that body — the
read path is presumably in `FUN_005a4770` (45 KB function; Ghidra's
decompiler times out, so we'll need radare2 or chopped re-decomp to spec
it).  Defer to Phase 2 `docs/formats/config-dat.md`.

> Engine on-disk byte ordering uses LE throughout (32-bit MSVC build).

## Defer-to-runtime trick (the harness bypass)

`tools/frida/opensummoners-agent.js` `installDialogBypass()` already
short-circuits this whole flow by replacing the engine's DLGPROC with a
wrapper that, on `WM_INITDIALOG`, force-checks Windowed (10020) + Disable
Sound (10024) and sends `BM_CLICK` to Launch (10003).  Knowing the scrape
path now confirms why that works: clicking 10003 sets `DAT_008a9a40=1`
and re-reads every radio into `DAT_008a9b48/4a/4c/4e` before `EndDialog`,
so the engine boots with those exact settings without persisting anything
to `config.dat` (the harness path never reaches the write code in
`FUN_005a4770`).

## Quirks worth folding into engine-quirks.md

### §5 — `Zoom Mode(1920x1440)` is always hidden

Control ID `0x272A` (10026) exists in the dialog resource (engine-quirks
§3 lists it) but `WM_INITDIALOG` calls `ShowWindow(…, SW_HIDE)` on it
unconditionally.  The radio scrape at 0x401424+ doesn't even check it.
So Zoom mode means only 1280x960 in this build, despite the resource
file suggesting otherwise.  Looks like 1920x1440 was either too
expensive on 2012-era hardware or never finished, and the dev removed
the toggle without ripping the control out of the .rc.

### §6 — Radio enums start at 3, not 0

Every saved radio uses **3 / 4 / 5** as the three possible values; the
"Disable Sound" checkbox writes 3 or 4.  Looks like the enum was
re-numbered at some point and they didn't rebase to zero — the same
"+3" offset appears in the on-disk config.dat layout.  When parsing
`config.dat` we have to faithfully reproduce this offset.

### §7 — Three controls are permanently disabled

IDs `0x271C`, `0x271D`, `0x271E` (10012/10013/10014) are `EnableWindow(false)`'d
at every `WM_INITDIALOG` with no path to ever re-enable them.  They are
visible-but-greyed in the dialog and probably correspond to abandoned
options.

## Files referenced

- `docs/decompiled/by-address/5b0d70.c` — DialogBoxParam call site.
- DLGPROC at `0x004013c0` — disassemble with
  `nix develop --command r2 -q -e scr.color=0 -c 'af @ 0x004013c0; pdf @ 0x004013c0' vendor/unpacked/sotes.unpacked.exe`
  (Ghidra missed it).
- `docs/decompiled/by-address/401730.c` — VRAM-group enable helper.
- `tools/frida/opensummoners-agent.js installDialogBypass()` — the
  harness-side short-circuit.
- `vendor/original/user/config.dat` — the persisted blob (XOR-obfuscated,
  layout TBD in Phase 2).
