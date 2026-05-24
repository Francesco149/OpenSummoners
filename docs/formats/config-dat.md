# Format spec — `user/config.dat`

Fortune Summoners persists per-user settings (launcher selections,
keybindings, volumes, …) in a single XOR-obfuscated file at
`user/config.dat` (relative to the game directory).

**Reference implementation:** `tools/extract/config_dat.py`.

## File shape

```
+--------+----------------+----------+
| 0x00   | u32 hdr_size   | = 16     |
| 0x04   | u32 version    | = 0x2711 |
| 0x08   | u32 data_size  | = 820    |
| 0x0C   | u32 checksum   | TBD      |
+--------+----------------+----------+
| 0x10   | body (data_size bytes, XOR-obfuscated; see below)         |
+--------+-----------------------------------------------------------+
| 0x10+data_size | tail (4 bytes, role TBD)                          |
+--------+-----------------------------------------------------------+
```

Total file size on the user's machine: **840 bytes** (16-byte header +
820 body + 4 tail).

### Header fields

| offset | type | name        | observed   | notes                                |
|--------|------|-------------|------------|--------------------------------------|
| 0x00   | u32  | `hdr_size`  | 16         | size of this header itself           |
| 0x04   | u32  | `version`   | 0x2711     | matches launcher dialog resource ID  |
| 0x08   | u32  | `data_size` | 820        | size of the obfuscated body          |
| 0x0C   | u32  | `checksum`  | 0x56db     | verification TBD — likely Adler-style|

The `version` value `0x2711 = 10001` is the same constant the engine
uses as the launcher dialog template resource ID (see
`docs/findings/launcher-dialog.md`).  Nice cross-confirmation that
this file belongs to the launcher's settings subsystem.

## XOR obfuscation

The 820-byte body is XOR'd with the single constant byte **`0x88`**.

The key was determined by inspection.  The file is full of `88 88 88
88` quad-byte runs because the schema reserves many u32 slots that
the user's actual settings leave at zero — and `0 ^ 0x88 = 0x88`.

To decode:

```python
plaintext = bytes(b ^ 0x88 for b in encoded_body)
```

## Body layout — what we know

The engine registers **101 schema fields** at boot via
`FUN_005afb90`, allocating a per-field record of 28 bytes
(`0x1c`).  In-memory total: `101 * 28 = 2828` bytes.  On-disk total:
820 bytes — i.e. the on-disk format is denser than the in-memory
representation, so the loader unpacks rather than memcpys.

### First dword

The plaintext starts with a 4-byte word that doesn't look like a
record:

```
00 plaintext: bc d9 34 b5
```

Plausibly a hash or per-file salt — TBD.

### Repeated pair structure

After the first dword the body is naturally parseable as a sequence
of **(u32, u32) pairs** — `(820 - 4) / 8 = 102` pairs exactly.

Each pair appears to be `(field_id, value)` or `(value, field_id)`
— the IDs cluster in obvious groups (`0x4300+` for one category,
`0x2A00+` for another, `0x8F00+` for a third) which suggests a flat
typed-key/value store rather than a positional struct.

```
plaintext  pair#  raw            interpretation TBD
0x14       0      04 00 00 00    field?
                  7d 8d eb 00    value?
0x1c       1      02 6b ef 00    field?
                  5b 00 00 00    value?
...
0x34       4      04 43 00 00    0x4304   ← id-cluster 0x4300+
                  18 00 00 00    24
0x3c       5      3e 8f 00 00    0x8F3E   ← id-cluster 0x8F00+
                  a4 00 00 00    0xA4
```

Field-ID-to-semantic mapping is TBD; will require either decompiling
`FUN_005a4770` (the 46 KB init function that runs the schema
registration + load) or hooking the runtime loader via Frida.

### Tail

The final 4 bytes after the obfuscated body (at file offsets
`0x344..0x348`) are not part of the body — possibly a second
checksum or signature.  Their role is TBD.

## Decoded sample (head of plaintext)

```
0x0010  bc d9 34 b5 04 00 00 00 7d 8d eb 00 02 6b ef 00
0x0020  5b 00 00 00 cc 00 00 00 30 2a 00 00 00 00 00 00
0x0030  04 43 00 00 18 00 00 00 3e 8f 00 00 a4 00 00 00
0x0040  68 8f 00 00 ef 00 00 00 c7 2a 00 00 18 00 00 00
0x0050  5f 43 00 00 00 00 00 00 05 43 00 00 00 00 00 00
0x0060  48 43 00 00 2a 00 00 00 8d 43 00 00 ef 00 00 00
0x0070  15 43 00 00 82 00 00 00 1e 43 00 00 82 00 00 00
```

(See `tools/extract/config_dat.py --raw` for the full dump on the
user's file.)

## Engine code map

| address      | role                                                |
|--------------|-----------------------------------------------------|
| `0x5a519a`   | call site that loads `config.dat`                    |
| `0x5b0ce0`   | path builder — "<game_dir>/<sub>/config.dat"         |
| `0x5afb00`   | schema array allocator — `new[101 * 28]`            |
| `0x5afb90`   | per-field schema register — stores at array index    |
| `FUN_005a4770` | 46 KB init function that hosts all of the above (Ghidra times out on it; use radare2 slices) |

## Engine quirks that touch this file

- §6 `Launcher radio enums start at 3, not 0` — the four launcher
  WORDs in this file use `3 / 4 / 5` for the radio positions.
- §8 `config.dat is XOR-obfuscated with a plaintext header` — the
  initial discovery entry.

## TODO

- Decode the magic / leading u32 (0xB534D9BC observed).
- Map field IDs (0x4300+, 0x2A00+, 0x8F00+, …) to engine semantics.
- Confirm checksum algorithm via Frida hook on the write path.
- Document the tail 4 bytes.
- Add a re-encoder once we need round-trip fidelity.
