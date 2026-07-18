// tools/sotes_trainer/trainer.c — standalone EN-SE trainer (injected DLL).
//
// In-process, no Frida.  Injected into sotes_en.exe (unpacked ImageBase 0x400000)
// via build/inject.exe (CreateProcess-SUSPENDED -> remote LoadLibrary -> Resume) or
// a version.dll proxy.  Hosts a localhost line-delimited JSON server (127.0.0.1:7777)
// that is BOTH the optional LLM interface and the stats-UI backend.  See DESIGN.md
// for the discovered mechanics + roadmap.
//
// Build: nix develop --command make -C tools/sotes_trainer  (-> build/sotes_trainer.dll;
//        links the standalone .sdt reader tools/sotes_save/).  See README.md.
//
// Protocol: one JSON object per line in, one per line out.  Examples:
//   {"cmd":"ping"}                          -> {"ok":true,"pong":true,"delta":...}
//   {"cmd":"player"}                        -> {"ok":true,"player":{...}}   (or null)
//   {"cmd":"read","addr":"0x92af80","type":"u32"}
//   {"cmd":"write","addr":"0x...","value":1,"type":"u32"}
//   {"cmd":"setstat","which":"hp","value":999,"lock":true}
//   {"cmd":"god","on":true}                 -> freeze hp+mp at max every tick
//   {"cmd":"teleport","x":50000,"y":40000}  -> write world_x/world_y (centi-px)

#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "sotes_save.h"      // standalone .sdt reader (tools/sotes_save/)
#include "MinHook.h"         // vendored inline-hook lib (tools/sotes_trainer/minhook, BSD-2)
#include "trainer_core.h"    // the typed C API the ImGui UI (trainer_ui.cpp) calls into

// ─── EN-SE addresses (unpacked ImageBase 0x400000) ──────────────────────────
#define ORIG_BASE     0x400000u
#define PLAYER_CODE   0xc35au        // Arche (entity code @ actor+0x1d4)
// actor-base offsets
#define OFF_CODE      0x1d4
#define OFF_HANDLE    0x1d8
// world_x/world_y are a DERIVED per-frame SNAPSHOT (re-committed from the phys-box
// every frame, so writing them is stomped in ~1 frame — the old teleport bug).  The
// AUTHORITATIVE position lives in the collision AABB the actor points to at +0x40.
// RE'd live off the position-commit at VA 0x484554 (HW write-watch on world_x):
//   world_x = box[+0x04]              (direct copy)   -> writing box[+4] STICKS
//   world_y = box[+0x08] + box[+0x10] - 1             (top + height - 1 = bottom)
#define OFF_WORLD_X   0xc76c         // centi-px (px*100) — DERIVED snapshot, read-only
#define OFF_WORLD_Y   0xc770         //                     DERIVED snapshot, read-only
#define OFF_BOX       0x40           // actor -> collision AABB ptr (authoritative pos)
#define BOX_X         0x04           // left  X  (= world_x)                 centi-px
#define BOX_TOP       0x08           // top   Y                              centi-px
#define BOX_W         0x0c           // width                               centi-px
#define BOX_H         0x10           // height (world_y = top + height - 1)  centi-px
#define OFF_STATBLOCK 0x760          // actor -> stat_block ptr
// stat_block offsets
#define OFF_HP_CUR    0x54
#define OFF_HP_BASE   0x58
#define OFF_HP_EQUIP  0x84
#define OFF_HP_BUFF   0x9c
#define OFF_MP_CUR    0x5c
#define OFF_MP_BASE   0x60
#define OFF_MP_EQUIP  0x88
#define OFF_MP_BUFF   0xa0
#define OFF_LEVEL     0xe0           // level_base (NOT the display level — see DESIGN
                                     // "Live findings": display Lv is EXP-derived)
#define OFF_EXP_CUR   0xec           // EXP progress   (SE offset; base-game +0xe8)
#define OFF_EXP_MAX   0xf0           // EXP to next    (cross-ref: 50000 = Arche Lv17)

#define TRAINER_PORT  7777

// ─── save-load chain + safepoint (EN-SE VAs, RE'd — see DESIGN.md session 3) ──
// The 3 load calls touch file IO / the game-state singleton / DirectDraw scene
// teardown, so they MUST run on the ENGINE thread at a per-frame safepoint, NOT the
// socket thread.  We hook the inputPoll (0x437c70, called constantly at the title) and
// drain a queued load there; and hook the title dispatcher (0x581ba0) to capture its
// `this` (= the scene-transition `this` the dispatcher passes to 0x5cb460).
#define VA_INPUTPOLL   0x437c70      // title/gameplay input poll = per-frame safepoint (5 bytes)
#define VA_MENUCTRL    0x4378d0      // generic-menu controller (the save-slot PICKER); 5 bytes
#define VA_DISPATCH    0x581ba0      // title dispatcher (ref only; not hooked)
#define BTN_CONFIRM    0x25          // confirm button id (37)
#define BTN_ROTATE     0x04          // menu rotate/next button id
#define VA_ALLOC       0x5ef121      // cdecl  alloc(size)              -> raw buffer
#define VA_SAVE_LOAD   0x416550      // thiscall(this=S, handle, slot, 0, 0) reads savedataNN.sdt
#define VA_SAVE_APPLY  0x586c60      // thiscall(this=singleton, handle, sel) apply into game state
#define VA_SCENE_TRANS 0x5cb460      // thiscall(this=dispatch, tgt, x, y) deferred scene transition
#define VA_GS_SINGLE   0x92ac68      // game-state singleton (apply's `this`, a static object)
#define VA_SCENE_THIS  0x92dd4c      // global holding the title scene handler `this` (0x5cb460's
                                     // `this`): the caller 0x58113e does `mov [0x92dd4c], ecx`
                                     // right before `call 0x581ba0` — read it, no hook needed.
#define VA_DEMO_JL     0x583866      // attract idle->demo trigger: jl 0x5832e1 -> patch to jmp
#define VA_DLGADV_JE   0x437740      // story-dialogue advance GATE: je 0x437752 (74 10) skips the
                                     // per-frame auto-advance when the flag [mgr+0x12c]==0.  NOP it
                                     // (90 90) -> the story stepper advances EVERY frame -> story/
                                     // cutscene/NPC dialogue skips itself (the "hold TAB" mechanism,
                                     // RE'd via a frida debugger: 73x wrap(cmd=8) all gated here).
#define VA_DLGGRID     0x5e59c0      // dialogue body TEXT-GRID ctor (thiscall, ecx=grid): allocs the
                                     // per-char cells (each 0x1b0 B, colored via 0x5e6630 w/ the body
                                     // colors 0x3e537d/0xa8b9cc) — shared by intro + in-game dialogue.
                                     // grid+0x4c=char total, +0x48=cell array, +0x4=active, +0x8=reveal
                                     // counter (init 0). 1st insn = 6-byte `mov %fs:0x0,%eax`.
#define SAVE_HANDLE_MAIN 0x2738      // Main-Quest save category (loader .sdt branch + apply gate)
#define SAVE_STRUCT_SZ   0xea94      // FUN_00585cf0 allocs this for the transient save struct

// ─── current-map / room chain (EN-SE, RE'd — see SE_CODE_MAP.md "CURRENT-MAP CHAIN") ──
// Found in the map-decode caller (0x5ad443: mov ecx,ds:0x92dd38; mov eax,[ecx+0x1038]):
//   render_root = *(0x92dd38); room_record = *(render_root + 0x1038)   (base +0x1038 carries)
// The room record (0x150 B) holds the CURRENT-room identity + its portal graph:
//   [0]=room_key, [1]=area, [3]=DATA/scene resource id (== the FindResource("DATA") map id),
//   [0x43]=tileset, [0x44]=parallax, and the EXITS (portals) at dword 7 stride 3 (20 slots):
//   dw(7+3k)=exit_key, dw(8+3k)=TARGET room key, dw(9+3k)=return/entry key.
// VERIFIED live: storage room 0x334dd/DATA 1026 -> 2 exits, both target 0x334dc (the shop).
// NOTE render_root+0x1044 (base map_obj) does NOT carry in SE — use the room record for identity.
#define VA_RENDER_ROOT  0x92dd38     // *(this) = render-root (room-state) object
#define ROOT_ROOM_REC   0x1038       // render_root + 0x1038 = current room record ptr
#define RR_ROOM_KEY     0x00         // room_record[0]
#define RR_AREA         0x04         // room_record[1]
#define RR_SCENE        0x0c         // room_record[3] = DATA/scene resource id
#define RR_TILESET      (0x43*4)     // room_record[0x43]
#define RR_PARALLAX     (0x44*4)     // room_record[0x44]
#define RR_EXIT0        0x1c         // room_record[7] = first exit slot (stride 0xc, 20 slots)
#define RR_EXIT_STRIDE  0xc          // per-exit stride: {exit_key, target_room, return_key}
#define RR_EXIT_TARGET  0x4          // exit slot +4 = TARGET room key (what a portal warps to)
#define RR_SIZE         0x150        // one room record = 0x150 B (the room table's stride)

// ── camera / mouse-fly (client cursor -> world) ──────────────────────────────
// The CAMERA/VIEW OBJECT = *(render_root + g_cam_off) (g_cam_off = base analog +0x104c, a POINTER
// in SE).  Its fields carry from base camera_follow.h (src/) — VERIFIED live (cal probes + the
// two-point teleport track): the view stores its own EASED scroll ORIGIN (top-left), not the
// player position, so the cursor mapping is stable:
//   cam+0x5c = cur_y (eased scroll top),  cam+0x60 = cur_x (eased scroll left)  [world cpx]
//   cam+0x64 = vp_w (64000 = 640px),      cam+0x68 = vp_h (48000 = 480px)
//   cam+0x6c = tgt_x,                     cam+0x70 = tgt_y  (the follow TARGET the easer chases)
// So the view spans a fixed 640x480 world-px window; a cursor at client fraction f maps to
// world = (cur_x, cur_y) + f*(vp_w, vp_h).  Using the EASED cur (not the instant player mirror
// at +0x14) is what stops the mouse-fly feedback runaway; the fly ALSO FREEZES the camera
// (pins cur/tgt to a latch) so the view can't scroll-follow her out from under the cursor.
#define CAM_MAP_W       0x00         // camera obj -> map pixel width  (cpx; for scroll clamping)
#define CAM_MAP_H       0x04         // camera obj -> map pixel height (cpx)
#define CAM_CUR_Y       0x5c         // camera obj -> eased scroll top  (world cpx) = view top
#define CAM_CUR_X       0x60         // camera obj -> eased scroll left (world cpx) = view left
#define CAM_VP_W        0x64         // camera obj -> viewport width  (cpx; 64000 = 640px)
#define CAM_VP_H        0x68         // camera obj -> viewport height (cpx; 48000 = 480px)
#define CAM_TGT_X       0x6c         // camera obj -> follow target x
#define CAM_TGT_Y       0x70         // camera obj -> follow target y
#define GOD_VAL         9999         // hp/mp pin value for god mode (USER spec: hp+mp 9999)
static volatile uint32_t g_cam_off   = 0x104c;   // render_root + this = camera/view-object POINTER
static volatile int      g_mousefly;             // continuous teleport-to-cursor (F7 / UI toggle)
static volatile int      g_fly_latched;          // camera-freeze latch valid this fly session
static volatile int      g_fly_fx, g_fly_fy;     // latched view top-left (frozen while flying)
static volatile HWND     g_game_hwnd;            // the CLASS_LIZSOFT_SOTES window (client-rect map)

static uintptr_t g_delta;            // actual base - ORIG_BASE (ASLR-safe)
static char      g_logpath[MAX_PATH];
#define AP(x) ((void *)((uintptr_t)(x) + g_delta))

static void vlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// ─── safe memory access (VirtualQuery-guarded) ──────────────────────────────
static int mem_readable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    DWORD prot = mbi.Protect & 0xff;
    if (prot == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD)) return 0;
    uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    return (uintptr_t)p + n <= end;
}
static int mem_writable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    DWORD w = mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY |
                             PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
    if (!w || (mbi.Protect & PAGE_GUARD)) return 0;
    uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    return (uintptr_t)p + n <= end;
}
static int rd32(const void *p, uint32_t *out) {
    if (!mem_readable(p, 4)) return 0;
    *out = *(volatile uint32_t *)p;
    return 1;
}

// ─── player-actor anchor (scan for the 0xc35a actor; cache + revalidate) ─────
static uintptr_t g_player;   // cached actor base (0 = unknown)

static int stat_max(uintptr_t sb, int base_off, int equip_off, int buff_off);
// Strong validation: code 0xc35a is common enough to collide, so cross-check that
// the whole stat block + world coords are mutually player-sane (rejects garbage
// false positives).  TODO(anchor): a ctor hook on 0x419b00 would be collision-proof.
static int actor_valid(uintptr_t base) {
    uint32_t code, sb, lvl, hpc, mpc, wx, wy;
    if (!rd32((void *)(base + OFF_CODE), &code) || code != PLAYER_CODE) return 0;
    if (!rd32((void *)(base + OFF_STATBLOCK), &sb) || !mem_readable((void *)sb, 0x200)) return 0;
    if (!rd32((void *)(sb + OFF_LEVEL), &lvl) || lvl < 1 || lvl > 99) return 0;
    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    if (hpmax < 1 || hpmax > 99999) return 0;
    if (mpmax < 0 || mpmax > 9999) return 0;
    // hp/mp_cur are NOT bounded by max: invincibility pins hp above max on purpose, so accept a
    // wide trainer range (the code/level/max/coord checks still reject false positives).
    if (!rd32((void *)(sb + OFF_HP_CUR), &hpc) || (int)hpc < 0 || (int)hpc > 999999) return 0;
    if (!rd32((void *)(sb + OFF_MP_CUR), &mpc) || (int)mpc < 0 || (int)mpc > 999999) return 0;
    if (!rd32((void *)(base + OFF_WORLD_X), &wx) || !rd32((void *)(base + OFF_WORLD_Y), &wy)) return 0;
    if (abs((int)wx) > 50000000 || abs((int)wy) > 50000000) return 0;   // centi-px sanity
    return 1;
}
static uintptr_t find_player(void) {
    if (g_player && actor_valid(g_player)) return g_player;   // cache hit = cheap (no scan)
    g_player = 0;
    // THROTTLE the cold full-address-space scan: callers poll this every frame (UI reads + the
    // engine-side freezes), and at a menu (no player) an every-frame full scan starves the game
    // loop AND raises the TOCTOU fault odds during a scene teardown.  When nothing is cached,
    // scan at most ~8x/sec; a freshly-spawned player is still picked up within ~120ms.
    static DWORD last_scan;
    DWORD now = GetTickCount();
    if (last_scan && (now - last_scan) < 120) return 0;
    last_scan = now;
    uint8_t *addr = 0;
    MEMORY_BASIC_INFORMATION mbi;
    while (VirtualQuery(addr, &mbi, sizeof mbi)) {
        uint8_t *next = (uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD) &&
            mbi.RegionSize <= 0x4000000) {
            uint32_t *p = (uint32_t *)mbi.BaseAddress;
            uint32_t *e = (uint32_t *)(next - 4);
            for (; p <= e; ++p) {
                if (*p != PLAYER_CODE) continue;
                uintptr_t base = (uintptr_t)p - OFF_CODE;
                if (actor_valid(base)) { g_player = base; return base; }
            }
        }
        if (next <= addr) break;
        addr = next;
    }
    return 0;
}

static int stat_max(uintptr_t sb, int base_off, int equip_off, int buff_off) {
    uint32_t b = 0, e = 0, f = 0;
    rd32((void *)(sb + base_off), &b);
    rd32((void *)(sb + equip_off), &e);
    rd32((void *)(sb + buff_off), &f);
    int m = (int)b + (int)e + (int)f;
    return m < 0 ? 0 : m;
}

// ─── generic memory scanner + dialogue-grid locator (hook-free "find the object") ──
// Walk committed MEM_PRIVATE RW regions (the game heap; MEM_IMAGE like sotesd.dll is
// skipped) collecting dword-aligned matches.  scan_u32 = the general value scan; the
// grid signature below locates the SE story/cutscene dialogue text-grid with NO hook
// (the input-mgr widget box_open is false for story dialogue — that box lives on a
// separate object built by the ctor 0x5e59c0).  Grid layout (RE'd, DESIGN §6c): +0x4
// active(=1), +0x8 reveal counter (0..total), +0x48 cell array, +0x4c char total; cells
// carry the body-text color 0x3e537d / 0xa8b9cc — the signature that rejects the many
// heap objects with a coincidental +0x4==1.
// ── SE dialogue reveal chain (RE'd session 8; DESIGN "Reveal counter FOUND") ──────
// g_dlg_grid (ctor-captured) is a text CONTAINER = a pool of line-widgets:
//   grid+0x48 = widget-ptr array; grid+0x4c = pool { cap:u16 = &0xffff, count:u16 = >>16 }.
//   widget (0x1b0 B) +0x170 -> the ACTIVE line's text-machine (0 once the line finishes;
//   the finalized TM then lives at widget+0x174).  TM (0x2c B): +0x10 total chars, +0x14
//   reveal cursor.  FAST-SKIP = write TM+0x14 = TM+0x10 (== port dialogue_skip_reveal; a
//   pure UI-STATE write, no input ⇒ can't trip a world interaction).  VERIFIED live: the
//   walk resolves (cap=10/count=7; active widget[6]+0x170->TM total=4 reveal=1) and forcing
//   +0x14=total sticks (the typewriter stops at reveal>=total).
#define GRID_WIDGET_ARR 0x48
#define GRID_POOL_CC    0x4c
#define WIDGET_TM       0x170
#define TM_TOTAL        0x10
#define TM_REVEAL       0x14

static int scan_u32(uint32_t needle, uintptr_t *out, int cap) {
    int n = 0;
    uint8_t *addr = 0; MEMORY_BASIC_INFORMATION mbi;
    while (n < cap && VirtualQuery(addr, &mbi, sizeof mbi)) {
        uint8_t *next = (uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD) &&
            mbi.RegionSize <= 0x8000000) {
            uint32_t *p = (uint32_t *)mbi.BaseAddress, *e = (uint32_t *)(next - 4);
            for (; p <= e && n < cap; ++p)
                if (*p == needle) out[n++] = (uintptr_t)p;
        }
        if (next <= addr) break;
        addr = next;
    }
    return n;
}
#define DLG_BODY_COLOR_A 0x3e537d
#define DLG_BODY_COLOR_B 0xa8b9cc
static int cell_has_body_color(uint32_t cells) {
    if (!mem_readable((void *)(uintptr_t)cells, 0x1b0)) return 0;
    for (uint32_t o = 0; o < 0x1b0; o += 4) {                 // (a) cells = INLINE cell array
        uint32_t v = 0;
        if (rd32((void *)(uintptr_t)(cells + o), &v) && (v == DLG_BODY_COLOR_A || v == DLG_BODY_COLOR_B)) return 1;
    }
    uint32_t c0 = 0;                                          // (b) cells = POINTER array -> cell[0]
    if (rd32((void *)(uintptr_t)cells, &c0) && mem_readable((void *)(uintptr_t)c0, 0x1b0)) {
        for (uint32_t o = 0; o < 0x1b0; o += 4) {
            uint32_t v = 0;
            if (rd32((void *)(uintptr_t)(c0 + o), &v) && (v == DLG_BODY_COLOR_A || v == DLG_BODY_COLOR_B)) return 1;
        }
    }
    return 0;
}
// A dialogue CONTAINER: +0x4 active(=1), +0x4c pool{cap,count}, +0x48 widget-ptr array with
// >=1 allocated line-widget carrying the body-text color (rejects unrelated heap pools).
static int grid_looks_valid(uintptr_t p) {
    uint32_t active = 0, cc = 0, arr = 0;
    if (!rd32((void *)(p + 0x4),  &active) || active != 1)                    return 0;
    if (!rd32((void *)(p + GRID_POOL_CC), &cc))                              return 0;
    uint32_t cap = cc & 0xffff, count = cc >> 16;
    if (cap < 1 || cap > 256 || count < 1 || count > cap)                    return 0;
    if (!rd32((void *)(p + GRID_WIDGET_ARR), &arr) || arr <= 0x10000)        return 0;
    for (uint32_t i = 0; i < count && i < 64; ++i) {
        uint32_t w = 0;
        if (rd32((void *)(uintptr_t)(arr + 4 * i), &w) && w > 0x10000 && cell_has_body_color(w))
            return 1;
    }
    return 0;
}
static int find_dlg_grids(uintptr_t *out, int cap) {
    int n = 0;
    uint8_t *addr = 0; MEMORY_BASIC_INFORMATION mbi;
    while (n < cap && VirtualQuery(addr, &mbi, sizeof mbi)) {
        uint8_t *next = (uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD) &&
            mbi.RegionSize <= 0x8000000) {
            uint32_t *p = (uint32_t *)mbi.BaseAddress, *e = (uint32_t *)(next - 0x50);
            for (; p <= e && n < cap; ++p) {
                if (*p != 1) continue;                        // cheap prefilter on +0x4==1
                uintptr_t base = (uintptr_t)p - 0x4;
                if (grid_looks_valid(base)) out[n++] = base;
            }
        }
        if (next <= addr) break;
        addr = next;
    }
    return n;
}

// Find the ACTIVE typing line's text-machine under container `grid` (walk the widget pool
// from the last-allocated line back; the current line is the one whose +0x170 TM is live).
// Returns the TM addr (0 if none), filling *total/*reveal/*widx.  DESIGN "Reveal counter FOUND".
static uint32_t dlg_active_tm(uint32_t grid, uint32_t *total, uint32_t *reveal, int *widx) {
    if (total)  *total  = 0;
    if (reveal) *reveal = 0;
    if (widx)   *widx   = -1;
    if (!grid) return 0;
    uint32_t cc = 0, arr = 0;
    if (!rd32((void *)(uintptr_t)(grid + GRID_POOL_CC), &cc))                    return 0;
    if (!rd32((void *)(uintptr_t)(grid + GRID_WIDGET_ARR), &arr) || arr <= 0x10000) return 0;
    uint32_t count = cc >> 16, cap = cc & 0xffff;
    if (cap < 1 || cap > 256 || count > cap)                                     return 0;
    for (int i = (int)count - 1; i >= 0; --i) {
        uint32_t w = 0, tm = 0, tot = 0, rev = 0;
        if (!rd32((void *)(uintptr_t)(arr + 4 * i), &w) || w <= 0x10000)         continue;
        if (!rd32((void *)(uintptr_t)(w + WIDGET_TM), &tm) || tm <= 0x10000)     continue;
        if (!rd32((void *)(uintptr_t)(tm + TM_TOTAL), &tot) || tot < 1 || tot > 8192) continue;
        if (!rd32((void *)(uintptr_t)(tm + TM_REVEAL), &rev))                    continue;
        if (total)  *total  = tot;
        if (reveal) *reveal = rev;
        if (widx)   *widx   = i;
        return tm;
    }
    return 0;
}
// Force every live typing line's reveal to total (pure UI-state skip).  Returns # skipped.
static int dlg_force_reveal(uint32_t grid) {
    if (!grid) return 0;
    uint32_t cc = 0, arr = 0;
    if (!rd32((void *)(uintptr_t)(grid + GRID_POOL_CC), &cc)) return 0;
    if (!rd32((void *)(uintptr_t)(grid + GRID_WIDGET_ARR), &arr) || arr <= 0x10000) return 0;
    uint32_t count = cc >> 16, cap = cc & 0xffff;
    if (cap < 1 || cap > 256 || count > cap) return 0;
    int forced = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t w = 0, tm = 0, tot = 0, rev = 0;
        if (!rd32((void *)(uintptr_t)(arr + 4 * i), &w) || w <= 0x10000)         continue;
        if (!rd32((void *)(uintptr_t)(w + WIDGET_TM), &tm) || tm <= 0x10000)     continue;
        if (!rd32((void *)(uintptr_t)(tm + TM_TOTAL), &tot) || tot < 1 || tot > 8192) continue;
        if (!rd32((void *)(uintptr_t)(tm + TM_REVEAL), &rev))                    continue;
        if (rev < tot && mem_writable((void *)(uintptr_t)(tm + TM_REVEAL), 4)) {
            *(volatile uint32_t *)(uintptr_t)(tm + TM_REVEAL) = tot;
            ++forced;
        }
    }
    return forced;
}

// ─── freeze table (applied every tick) ──────────────────────────────────────
static volatile int g_god = 1;             // freeze hp+mp at GOD_VAL (9999); DEFAULT ON (USER)
static volatile int g_fastskip;            // fast-skip: force the dialogue typewriter reveal to total
static volatile uint32_t g_dlg_grid;       // story/cutscene text-grid `this` (also declared below near
                                           // the dlgtrace; a tentative def — set by dlggrid/fastskip scan)
#define MAX_LOCKS 16
static struct { int used; uintptr_t addr; uint32_t val; } g_locks[MAX_LOCKS];
static CRITICAL_SECTION g_lock_cs;

static void lock_set(uintptr_t addr, uint32_t val) {
    EnterCriticalSection(&g_lock_cs);
    int free = -1;
    for (int i = 0; i < MAX_LOCKS; ++i) {
        if (g_locks[i].used && g_locks[i].addr == addr) { g_locks[i].val = val; LeaveCriticalSection(&g_lock_cs); return; }
        if (!g_locks[i].used && free < 0) free = i;
    }
    if (free >= 0) { g_locks[free].used = 1; g_locks[free].addr = addr; g_locks[free].val = val; }
    LeaveCriticalSection(&g_lock_cs);
}
static void lock_clear_all(void) {
    EnterCriticalSection(&g_lock_cs);
    for (int i = 0; i < MAX_LOCKS; ++i) g_locks[i].used = 0;
    LeaveCriticalSection(&g_lock_cs);
}

// NOTE: the periodic freezes (invincibility / god / setstat locks / fastskip) used to run on a
// standalone 50ms tick_thread that POKED engine memory off-thread.  They now run ENGINE-SIDE in
// the inputPoll safepoint (engine_freezes, called from poll_title_cb) — see the engine-thread
// execution queue below.  No worker thread pokes engine memory anymore.

// The engine-thread safepoint work (defined with the execution queue, below) — forward-declared
// so poll_title_cb can drain the queue + run the per-frame freezes/mouse-fly/hotkeys.
static void drain_engine_queue(void);
static void engine_freezes(void);
static void engine_mousefly(void);
static void engine_hotkeys(void);

// ─── tiny JSON helpers (line protocol) ──────────────────────────────────────
// Extract a string value for "key". Returns len (0 if absent). Not a full parser.
static int json_str(const char *s, const char *key, char *out, int cap) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return 0;
    p = strchr(p + strlen(pat), ':'); if (!p) return 0;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return 0;
    ++p; int n = 0;
    while (*p && *p != '"' && n < cap - 1) out[n++] = *p++;
    out[n] = 0; return n;
}
// Extract a numeric value (dec or 0x hex) for "key". Returns 1 if found.
static int json_num(const char *s, const char *key, long long *out) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return 0;
    p = strchr(p + strlen(pat), ':'); if (!p) return 0;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    if (*p == '"') ++p;                 // allow "0x.." quoted
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) base = 16;
    *out = strtoll(p, NULL, base); return 1;
}
static int json_bool(const char *s, const char *key, int dflt) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return dflt;
    p = strchr(p + strlen(pat), ':'); if (!p) return dflt;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    return strncmp(p, "true", 4) == 0 || *p == '1';
}

static uint32_t read_typed(uintptr_t a, const char *ty) {
    if (!strcmp(ty, "u8"))  { uint32_t v = 0; if (mem_readable((void*)a,1)) v = *(uint8_t*)a;  return v; }
    if (!strcmp(ty, "u16")) { uint32_t v = 0; if (mem_readable((void*)a,2)) v = *(uint16_t*)a; return v; }
    uint32_t v = 0; rd32((void *)a, &v); return v;
}
static void write_typed(uintptr_t a, uint32_t v, const char *ty) {
    if (!strcmp(ty, "u8"))  { if (mem_writable((void*)a,1)) *(uint8_t*)a  = (uint8_t)v;  return; }
    if (!strcmp(ty, "u16")) { if (mem_writable((void*)a,2)) *(uint16_t*)a = (uint16_t)v; return; }
    if (mem_writable((void *)a, 4)) *(uint32_t *)a = v;
}

// ─── engine-function caller ─────────────────────────────────────────────────
// Call fn(args[0..n-1]) with ecx=ecx_this (harmless for cdecl/stdcall — ecx is
// caller-saved; only thiscall reads it).  esp is saved before and restored after
// the call, so ANY calling convention (cdecl/stdcall/thiscall) is safe — a
// stdcall/thiscall `ret N` self-cleanup and a cdecl caller-cleanup both reduce to
// "esp := saved".  O0 keeps ebp a frame pointer so the ebp-relative "m" operands
// stay valid across the arg pushes.  Power primitive: unblocks the load chain.
static uint32_t __attribute__((noinline, optimize("O0")))
call_va(void *fn, uint32_t ecx_this, const uint32_t *args, int n) {
    volatile uint32_t ret = 0;
    __asm__ __volatile__(
        "movl %%esp, %%ebx        \n\t"   // save esp
        "movl %[n], %%edx         \n\t"   // edx = n (index, counts down)
        "1: testl %%edx, %%edx    \n\t"
        "   jz 2f                 \n\t"
        "   decl %%edx            \n\t"
        "   movl %[args], %%eax   \n\t"   // reload args base (ebp-relative, safe)
        "   pushl (%%eax,%%edx,4) \n\t"   // push args[edx]  (right-to-left)
        "   jmp 1b                \n\t"
        "2: movl %[ecx], %%ecx    \n\t"   // ecx = this
        "   call *%[fn]           \n\t"
        "   movl %%ebx, %%esp     \n\t"   // restore esp (convention-agnostic cleanup)
        "   movl %%eax, %[ret]    \n\t"
        : [ret] "=m" (ret)
        : [fn] "m" (fn), [ecx] "m" (ecx_this), [args] "m" (args), [n] "m" (n)
        : "eax", "ebx", "ecx", "edx", "cc", "memory");
    return ret;
}

// Build the player JSON into out. Returns 0 if no player.
static int player_json(char *out, int cap) {
    uintptr_t base = find_player();
    if (!base) { snprintf(out, cap, "null"); return 0; }
    uint32_t wx=0, wy=0, sb=0, hp=0, mp=0, lvl=0, ec=0, em=0;
    rd32((void *)(base + OFF_WORLD_X), &wx);
    rd32((void *)(base + OFF_WORLD_Y), &wy);
    rd32((void *)(base + OFF_STATBLOCK), &sb);
    rd32((void *)(sb + OFF_HP_CUR), &hp);
    rd32((void *)(sb + OFF_MP_CUR), &mp);
    rd32((void *)(sb + OFF_LEVEL), &lvl);
    rd32((void *)(sb + OFF_EXP_CUR), &ec);
    rd32((void *)(sb + OFF_EXP_MAX), &em);
    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    // level_base (0xe0) is NOT the display level (that is EXP-derived — DESIGN);
    // exp_cur/exp_max are exposed so the true level can be looked up from a table.
    snprintf(out, cap,
        "{\"actor\":\"0x%08x\",\"world_x\":%d,\"world_y\":%d,\"stat_block\":\"0x%08x\","
        "\"hp\":%d,\"hp_max\":%d,\"mp\":%d,\"mp_max\":%d,\"level_base\":%d,"
        "\"exp_cur\":%d,\"exp_max\":%d}",
        (unsigned)base, (int)wx, (int)wy, (unsigned)sb,
        (int)hp, hpmax, (int)mp, mpmax, (int)lvl, (int)ec, (int)em);
    return 1;
}

// ─── engine-thread safepoint + save-load chain ──────────────────────────────
// The load calls run from a hook on the inputPoll (a per-frame engine-thread
// safepoint), never the socket thread.  0x581ba0's `this` (the scene object the
// dispatcher hands 0x5cb460) is captured by a second hook.
static volatile uint32_t g_dispatch_this;   // scene `this` read from 0x92dd4c at load time
static volatile int      g_load_pending, g_load_slot, g_load_enter, g_in_safepoint;
static volatile uint32_t g_load_ret;
static char              g_load_log[192];

// ─── menu-drive (the VERIFIED load path): inject input records into the game's own
// menu managers.  Title polls 0x437c70, the save-slot picker polls 0x4378d0; each
// trampoline captures its poll esp+ecx(=manager) so the callback can time the record
// to the poll's own `now` (match window 0x64) and target the live ring.  See DESIGN.md.
static volatile uint32_t g_ti_esp, g_ti_mgr;   // title-poll  (0x437c70) captured esp / manager
static volatile uint32_t g_pk_esp, g_pk_mgr;   // picker-poll (0x4378d0) captured esp / manager
static volatile int      g_md_state;           // 0 idle, 1 want title-confirm, 2 want picker-confirm
static volatile int      g_md_downs;           // picker rotations before confirm (0 = default slot)
static volatile int      g_md_slot = -1;       // target save slot (-1 = default/newest highlight)
static volatile int      g_dlgskip = 1;        // auto-skip dialogue (DEFAULT ON)
// dlgskip auto-advances an OPEN dialogue box hands-free.  It gates on a PASSIVE read of the SE
// dialogue widget (dialogue_box_open below) — NOT the old active 0x24-probe consumption gate.
// WHY the rewrite (session 6): the old gate injected 0x24/0x27 EVERY gameplay poll to *detect*
// a dialogue (watch the probe get consumed).  That active probing ran in freeroam too and — with
// a 1-poll detection lag — leaked a world input on close, AUTO-TRIGGERING the house-exit door
// (the "Dad needs help" gate fires on UP-into-the-door; it does not normally auto-open).  The fix
// per USER: DISTINGUISH an already-open box by READING the widget, and only inject while it is up.
// So in freeroam we now inject NOTHING (no probe) → the door is never auto-triggered.
// The widget lives on the input manager (g_ti_mgr): DLG_BOX_CELL0 (+0x374) = the first content-cell
// ptr (0 ⇒ no box on screen), DLG_BOX_SCALE (+0x3a4) = the pop-in scale (0..1000, 1000 = fully
// open).  The box object itself = *(mgr+0x374) with the layout +0x4 active / +0x54 scale.  RE'd
// live (session 6): freeroam↔dialogue differential off *(0x92dd4c)/the input mgr; box-frame /
// body-text color constants (0x3e537d/0xa8b9cc) located the SE dialogue code in
// vendor/unpacked/editions/sotes-ense-en.exe.  See DESIGN.md "Dialogue-skip — passive gate".
#define DLG_BOX_CELL0   0x374   // input-mgr -> dialogue widget: first content-cell ptr (0 = closed)
#define DLG_BOX_SCALE   0x3a4   // input-mgr -> dialogue widget: pop-in scale (0..1000)
#define DLG_SETTLE_POLLS 120    // hold dlgskip off ~2s after a menu-drive (scene-settle debounce)
static volatile int      g_dlg_btns[6] = { 0, 0, 0, 0, 0, 0 };  // extra reveal-skip ids (0=unused;
                                 // default EMPTY — the reveal-skip id doubles as world UP, so it
                                 // must stay off until the reveal is done via a UI-STATE write,
                                 // per USER "force UI state, not input").  PORT-DEBT(dlgskip-reveal-ui).
// The dialogue text CONTAINER `this`, captured at the 0x5e59c0 ctor entry (hook precb = mov
// [g_dlg_grid],ecx).  Shared by intro + in-game dialogue, so this is THE handle to the reveal
// state for ANY box: a pool of line-widgets at grid+0x48 (count @ +0x4e), each line's typewriter
// text-machine at widget+0x170 (total @ TM+0x10, reveal @ TM+0x14).  Walk via dlg_active_tm /
// force via dlg_force_reveal.  0 until the first dialogue line builds a container.
static volatile uint32_t g_dlg_grid;
// ── dialogue-field TRACE (temporary RE aid) ── while g_dlg_grid is active, snapshot grid[0..0xC0]
// every poll from the moment a NEW grid is built (engine thread ⇒ catches every typewriter frame
// the socket is too slow to poll).  Read via `dlgtrace` — the column that climbs first→last is the
// reveal counter, a sibling holds the total.  Covers ALL dialogue (the grid ctor is shared).
#define DLGT_W     48       // grid u32s (0x000..0x0C0)
#define DLGT_N     80       // frames
static uint32_t          g_dlgt[DLGT_N][DLGT_W];
static volatile int      g_dlgt_n;      // frames captured this grid
static uint32_t          g_dlgt_lastgrid;
static volatile int      g_ng_state;            // new-game drive: 0 idle, 1 rotate-to-Start, 2 confirm
static volatile int      g_ng_rotates;          // title rotations to reach the New Game item
static volatile int      g_ng_btn = 0x04;       // title rotate button (2=up / 4=down)
static volatile int      g_ng_gap, g_ng_cool;   // rotate cadence: 1 rotate every g_ng_gap polls

// Save-slot picker selection (getters FUN_005e8e80/FUN_005e8ea0): the save-list MANAGER
// holds the slot list at *(mgr+0x17c) — 0x10-byte entries { handle@+0, slot@+4 } (slot =
// the savedataNN index) — and the selected index at *( *(mgr+0x174) + 0x14 ).  The MANAGER
// is NOT the 0x4378d0 controller ecx; it is the controller's arg1 (VERIFIED live: poll
// esp+4).  We probe ecx + stack args 1..5 for the object whose +0x17c list's first entry
// has a small savedata slot id (garbage candidates are skipped), then write the index of
// the entry with slot==target.
#define PK_SEL_MODEL  0x174   // manager -> selection-model ptr (+0x14 = selected index)
#define PK_LIST_BASE  0x17c   // manager -> slot-list base ptr  (entries {handle@0, slot@4})
static char         g_pk_diag[128];   // concise pick result, surfaced in the `load` response
static volatile int g_pk_diag_done;
// Point the picker at savedataNN==`target`; returns the chosen list index, or -1 (no write)
// if not found — the caller then loads the default (newest) highlight.  Only writes after a
// matching in-bounds entry in a validated list, so a wrong candidate is a safe no-op.
static int picker_select_slot(uint32_t mgr, uint32_t esp, int target) {
    uint32_t cand[8]; int nc = 0;
    cand[nc++] = mgr;                                              // the controller ecx
    if (esp) for (int a = 1; a <= 5; ++a) { uint32_t v = 0;       // + its stack args 1..5
        if (rd32((void *)(uintptr_t)(esp + (uint32_t)a * 4), &v)) cand[nc++] = v; }
    for (int c = 0; c < nc; ++c) {
        uint32_t sm = 0, lb = 0, s0 = 0;
        if (!rd32((void *)(uintptr_t)(cand[c] + PK_SEL_MODEL), &sm) || !sm) continue;
        if (!rd32((void *)(uintptr_t)(cand[c] + PK_LIST_BASE), &lb) || !lb) continue;
        if (!rd32((void *)(uintptr_t)(lb + 4), &s0) || s0 > 15) continue;   // not the manager
        for (int i = 0; i < 16; ++i) {                            // validated list: find target
            uint32_t sf = 0;
            if (!rd32((void *)(uintptr_t)(lb + (uint32_t)i * 0x10 + 4), &sf)) break;
            if ((int)sf == target) {
                if (mem_writable((void *)(uintptr_t)(sm + 0x14), 4))
                    *(volatile uint32_t *)(uintptr_t)(sm + 0x14) = (uint32_t)i;
                if (!g_pk_diag_done) { snprintf(g_pk_diag, sizeof g_pk_diag, "slot %d -> list index %d", target, i); g_pk_diag_done = 1; }
                return i;
            }
        }
    }
    if (!g_pk_diag_done) { snprintf(g_pk_diag, sizeof g_pk_diag, "slot %d not found in picker (loaded default)", target); g_pk_diag_done = 1; }
    return -1;
}

#define IN_POOL 64
static uint8_t           g_inpool[IN_POOL * 12];
static volatile unsigned g_inpool_idx;
// Write one input record {+0=id,+4=ts,+8=1} into a trainer-owned pool and publish its
// pointer into the manager ring at mgr[0x0c + slot*4] (slot 63 = first polled).  The
// game's poll (0x437c70/0x4378d0) matches +0==polled_id && +8==1 && now-ts<=0x64.
static uint8_t *inject_record(uint32_t mgr, uint32_t id, uint32_t now, int ordinal) {
    if (!mgr || !mem_writable((void *)(uintptr_t)mgr, 0x10c)) return NULL;
    uint8_t *rec = g_inpool + (g_inpool_idx++ % IN_POOL) * 12;
    *(volatile uint32_t *)(rec + 0) = id;
    *(volatile uint32_t *)(rec + 4) = now;
    *(volatile uint32_t *)(rec + 8) = 1;
    int slot = 63 - (ordinal & 63);
    *(volatile uint32_t *)(uintptr_t)(mgr + 0x0c + slot * 4) = (uint32_t)(uintptr_t)rec;
    return rec;   // caller may watch rec[+0]: a poll clears it to 0 on consume (dialogue-active probe)
}

// PASSIVE dialogue-open test: read the SE dialogue widget (embedded in the input mgr) and report
// whether an IN-GAME dialogue box is on screen.  No injection — the non-lagged replacement for the
// old 0x24-probe consumption gate (see the g_dlgskip comment).  Two conditions:
//   (1) cell0 (+0x374) != 0 — the widget's first content-cell ptr (0 in freeroam);
//   (2) scale (+0x3a4) in [1,1000] — the pop-in scale of a real box.
// (2) is what rejects the TITLE: there g_ti_mgr is a DIFFERENT object (the title/menu manager,
// not the in-game input mgr that owns the dialogue widget), where +0x374 holds a menu ptr and
// +0x3a4 aliases a heap pointer (huge, ≫1000).  Freeroam has +0x374==0 && scale==0; only an
// actual dialogue box has both cell0!=0 AND a sane 1..1000 scale.  (Verified live: title
// +0x3a4=0x49600c0, freeroam=0, open=1000.)
static int dialogue_box_open(uint32_t mgr) {
    uint32_t cell0 = 0, scale = 0;
    if (!mgr) return 0;
    if (!rd32((void *)(uintptr_t)(mgr + DLG_BOX_CELL0), &cell0) || !cell0) return 0;
    if (!rd32((void *)(uintptr_t)(mgr + DLG_BOX_SCALE), &scale)) return 0;
    return scale >= 1 && scale <= 1000;
}

// Runs on the ENGINE thread (from the inputPoll safepoint).  Reproduces the retail
// Continue-confirm terminal: alloc a save struct -> load savedataNN.sdt -> apply into
// the game-state singleton -> deferred scene transition.  Every step is logged so a
// failing step is identifiable from the `load` response.
static void do_load_at_safepoint(int slot) {
    void *fn_alloc = AP(VA_ALLOC), *fn_load = AP(VA_SAVE_LOAD),
         *fn_apply = AP(VA_SAVE_APPLY), *fn_trans = AP(VA_SCENE_TRANS);
    if (!mem_readable(fn_alloc, 1) || !mem_readable(fn_load, 1)) {
        snprintf(g_load_log, sizeof g_load_log, "engine fns unreadable"); return; }
    uint32_t sz = SAVE_STRUCT_SZ;
    uint32_t S = call_va(fn_alloc, 0, &sz, 1);                 // cdecl alloc(0xea94)
    if (!S || !mem_writable((void *)(uintptr_t)S, SAVE_STRUCT_SZ)) {
        snprintf(g_load_log, sizeof g_load_log, "alloc(0x%x) failed S=0x%x", SAVE_STRUCT_SZ, S); return; }
    memset((void *)(uintptr_t)S, 0, SAVE_STRUCT_SZ);
    uint32_t la[4] = { SAVE_HANDLE_MAIN, (uint32_t)slot, 0, 0 };
    uint32_t r = call_va(fn_load, S, la, 4);                   // thiscall load -> savedataNN.sdt
    g_load_ret = r;
    if (!r) { snprintf(g_load_log, sizeof g_load_log,
        "416550 ret 0 (savedata%02d.sdt missing/invalid); S=0x%x", slot, S); return; }
    uint32_t aa[2] = { SAVE_HANDLE_MAIN, (uint32_t)slot };
    call_va(fn_apply, (uint32_t)(uintptr_t)AP(VA_GS_SINGLE), aa, 2);   // apply into the singleton
    uint32_t dt = 0; rd32(AP(VA_SCENE_THIS), &dt); g_dispatch_this = dt;
    if (!g_load_enter) {
        snprintf(g_load_log, sizeof g_load_log,
            "slot=%d loaded+applied ret=%u S=0x%x this=0x%x (enter=0: transition skipped)",
            slot, r, S, dt);
        return;
    }
    // EXPERIMENTAL: the raw 0x5cb460 transition still crashes — likely missing the
    // pre-transition setup 0x585cf0 does (validate 0x57f020 / target scene build) or a
    // wrong apply `sel`.  Args match the Start path 5cb460(this,0x2724,0,0); needs the
    // observed real Continue-load args to finish.  See DESIGN.md session 3.
    if (dt && mem_readable((void *)(uintptr_t)dt, 4) && mem_readable(fn_trans, 1)) {
        uint32_t ta[3] = { SAVE_HANDLE_MAIN, 0, 0 };
        call_va(fn_trans, dt, ta, 3);                         // deferred scene transition
        snprintf(g_load_log, sizeof g_load_log,
            "OK slot=%d ret=%u S=0x%x this=0x%x -> transition attempted (EXPERIMENTAL)", slot, r, S, dt);
    } else {
        snprintf(g_load_log, sizeof g_load_log,
            "slot=%d loaded+applied ret=%u S=0x%x but dispatch_this=0x%x invalid - no transition",
            slot, r, S, dt);
    }
}

// Title-poll (0x437c70) callback — runs on the engine thread each poll.  Drains the
// experimental direct-call load, and injects the title confirm for the menu-drive.
static void __attribute__((cdecl)) poll_title_cb(void) {
    // Engine-thread safepoint: run the queued pokes/calls + the per-frame freezes/mouse-fly/hotkeys
    // HERE (in the game loop), so no worker thread ever touches engine state (USER).
    engine_hotkeys();
    drain_engine_queue();
    engine_freezes();
    engine_mousefly();
    if (g_load_pending && !g_in_safepoint) {          // experimental direct chain (loadraw)
        g_in_safepoint = 1; int slot = g_load_slot; g_load_pending = 0;
        do_load_at_safepoint(slot); g_in_safepoint = 0;
    }
    uint32_t now = 0, e = g_ti_esp;
    if (e) rd32((void *)(uintptr_t)(e + 4), &now);        // 0x437c70 arg1 = now
    if (g_md_state == 1 && g_ti_mgr) {                // menu-drive step 1: confirm at the title
        // Re-inject the title confirm EVERY poll until the save-slot picker opens (its poll
        // 0x4378d0 sets g_pk_mgr; the title only polls 0x437c70, so g_pk_mgr==0 until then).
        // A single title confirm is sometimes dropped before the title menu settles — the
        // old one-shot advance-to-2 then hung with the picker never open.
        if (g_pk_mgr) { g_md_state = 2; return; }     // picker up -> picker-confirm step
        inject_record(g_ti_mgr, BTN_CONFIRM, now, 0);
    }
    if (g_ng_state && g_ti_mgr) {                     // new-game drive: rotate to Start, then confirm
        if (g_ng_state == 1) {
            if (g_ng_rotates > 0) {                   // one rotate every g_ng_gap polls (menu settle)
                if (g_ng_cool > 0) g_ng_cool--;
                else { inject_record(g_ti_mgr, (uint32_t)g_ng_btn, now, 3); g_ng_rotates--; g_ng_cool = g_ng_gap; }
            } else g_ng_state = 2;
        } else {                                      // step 2: confirm (re-inject until scene leaves title)
            inject_record(g_ti_mgr, BTN_CONFIRM, now, 3);
        }
    }
    // Dialogue-field TRACE (RE aid): while the hooked text-grid (g_dlg_grid, from the 0x5e59c0
    // ctor) is active, snapshot grid[0..0xC0] every poll from the moment a NEW grid is built.
    // Independent of g_dlgskip so a naturally-typing box is caught (intro or in-game).
    {
        uint32_t g = g_dlg_grid, act = 0;
        if (g && rd32((void *)(uintptr_t)(g + 4), &act) && act) {
            if (g != g_dlgt_lastgrid) { g_dlgt_n = 0; g_dlgt_lastgrid = g; }   // new grid -> fresh trace
            if (g_dlgt_n < DLGT_N && mem_readable((void *)(uintptr_t)g, DLGT_W * 4)) {
                uint32_t *row = g_dlgt[g_dlgt_n];
                for (int i = 0; i < DLGT_W; i++)
                    row[i] = *(volatile uint32_t *)(uintptr_t)(g + i * 4);
                g_dlgt_n++;
            }
        }
    }
    // Scene-settle debounce: a freshly-loaded scene (right after a newgame/load menu-drive) is
    // briefly UNSTABLE — touching the dialogue widget the instant the drive ends faults the game.
    // (PRE-EXISTING: dlgskip-on through `newgame` crashed BOTH this build and the pre-passive-gate
    // one — the transition window, not the gate logic; proven by rebuilding e92abf9.)  Hold dlgskip
    // off for a beat after any drive so the scene settles first.
    static int g_dlg_settle = 0;
    if (g_md_state || g_ng_state) g_dlg_settle = 0;
    else if (g_dlg_settle < 100000)  g_dlg_settle++;
    // Auto-skip dialogue (suppressed while WE drive a menu / until the scene settles).  GATE: only
    // when a box is actually on screen (dialogue_box_open, a passive read of the widget) — so in
    // freeroam we inject NOTHING and can't trigger a world interaction (the door).  When a box IS up,
    // inject the advance ids 0x24/0x27 (dialogue-only, inert in the world) so each box steps hands-
    // free; the reveal-skip ids (g_dlg_btns, default EMPTY) also fire — but ONLY while the box is up,
    // and because the read is non-lagged they stop the instant it closes (no leak on close).
    if (g_dlgskip && g_ti_mgr && !g_md_state && !g_ng_state && g_dlg_settle > DLG_SETTLE_POLLS
        && dialogue_box_open(g_ti_mgr)) {
        inject_record(g_ti_mgr, 0x24, now, 4);                     // advance (dialogue-only)
        inject_record(g_ti_mgr, 0x27, now, 5);                     // advance (dialogue-only)
        for (int i = 0; i < 6; ++i)
            if (g_dlg_btns[i]) inject_record(g_ti_mgr, (uint32_t)g_dlg_btns[i], now, 6 + i);
    }
}

// Picker-poll (0x4378d0) callback — fires only while the save-slot picker is up.  If a
// target slot is set, point the picker's selection at it each poll (idempotent) BEFORE
// confirming; else apply any manual rotations first.  Then keep re-injecting confirm
// every poll until the load fires and the picker goes away (a single early confirm is
// dropped before the cursor settles; the load command ends the drive once find_player()).
static void __attribute__((cdecl)) poll_picker_cb(void) {
    if (g_md_state != 2 || !g_pk_mgr) return;         // menu-drive step 2: navigate + confirm
    uint32_t now = 0, e = g_pk_esp;
    if (e) rd32((void *)(uintptr_t)(e + 8), &now);        // 0x4378d0 arg2 = now
    if (g_md_slot >= 0) picker_select_slot(g_pk_mgr, g_pk_esp, g_md_slot);   // target a specific slot
    else if (g_md_downs > 0) { inject_record(g_pk_mgr, BTN_ROTATE, now, 0); g_md_downs--; return; }
    inject_record(g_pk_mgr, BTN_CONFIRM, now, 0);
}

// ─── minimal inline hooks (runtime-codegen trampolines) ──────────────────────
static uint8_t *g_tramp_p;

static void patch_bytes(void *dst, const void *src, size_t n) {
    DWORD old = 0;
    if (!VirtualProtect(dst, n, PAGE_EXECUTE_READWRITE, &old)) return;
    memcpy(dst, src, n);
    VirtualProtect(dst, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), dst, n);
}

// Detour `target` (overwriting `savelen` whole-instruction bytes, >=5) to a trampoline
// that: [if cb] pushad/pushfd, 16-align esp, call cb, restore; [if precode] emit it;
// run the saved original bytes; jmp back to target+savelen.  The saved bytes must be
// position-independent (verified: 0x437c70/0x581ba0 prologues are).
static void install_detour(uintptr_t target, int savelen, void *cb,
                           const uint8_t *precb, int precblen) {
    uint8_t *t = g_tramp_p;
    if (precb && precblen) { memcpy(g_tramp_p, precb, (size_t)precblen); g_tramp_p += precblen; }
    if (cb) {
        *g_tramp_p++ = 0x60;                                   // pushad
        *g_tramp_p++ = 0x9c;                                   // pushfd
        *g_tramp_p++ = 0x89; *g_tramp_p++ = 0xe5;              // mov ebp,esp
        *g_tramp_p++ = 0x83; *g_tramp_p++ = 0xe4; *g_tramp_p++ = 0xf0;  // and esp,-16
        *g_tramp_p++ = 0xe8;                                   // call rel32 -> cb
        int32_t rel = (int32_t)((uintptr_t)cb - ((uintptr_t)g_tramp_p + 4));
        memcpy(g_tramp_p, &rel, 4); g_tramp_p += 4;
        *g_tramp_p++ = 0x89; *g_tramp_p++ = 0xec;              // mov esp,ebp
        *g_tramp_p++ = 0x9d;                                   // popfd
        *g_tramp_p++ = 0x61;                                   // popad
    }
    memcpy(g_tramp_p, (void *)target, (size_t)savelen); g_tramp_p += savelen;   // saved prologue
    *g_tramp_p++ = 0xe9;                                       // jmp rel32 -> target+savelen
    int32_t rb = (int32_t)((target + savelen) - ((uintptr_t)g_tramp_p + 4));
    memcpy(g_tramp_p, &rb, 4); g_tramp_p += 4;
    FlushInstructionCache(GetCurrentProcess(), t, (SIZE_T)(g_tramp_p - t));
    uint8_t patch[16];
    patch[0] = 0xe9;                                           // jmp rel32 -> trampoline
    int32_t rt = (int32_t)((uintptr_t)t - (target + 5));
    memcpy(patch + 1, &rt, 4);
    for (int i = 5; i < savelen; ++i) patch[i] = 0x90;        // nop tail
    patch_bytes((void *)target, patch, (size_t)savelen);
}

// Suspend/resume every OTHER thread of this process — closes the race where the engine
// executes a half-written 5-byte detour during install.
static void suspend_others(int suspend) {
    DWORD me = GetCurrentThreadId(), pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te; te.dwSize = sizeof te;
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid && te.th32ThreadID != me) {
                HANDLE h = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (h) { if (suspend) SuspendThread(h); else ResumeThread(h); CloseHandle(h); }
            }
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
}

// ─── MinHook inline hook on the dialogue grid ctor (robust grid capture) ─────
// The SEH-safe way to grab the story/cutscene grid `this` at BUILD time (every box, typing or
// waiting — retires the fragile dlggrid scan).  MinHook's HDE length disassembler copies the WHOLE
// 6-byte `mov eax,fs:0x0` prologue (the fixed-5-byte install_detour truncated it → corruption).
// The ctor 0x5e59c0 is __thiscall(ecx=grid, 7 stack args, ret 0x1c); the detour captures ecx →
// g_dlg_grid then calls the original (trampoline) with the same 7 args (each arg-set cleaned once —
// no double-ret).  A pure inline hook, no VEH, so it does NOT perturb the intro's C++/SEH exceptions
// (unlike the dropped HW-breakpoint engine).
typedef void (__attribute__((__thiscall__)) *dlg_ctor_fn)(void *self, uint32_t, uint32_t, uint32_t,
                                                          uint32_t, uint32_t, uint32_t, uint32_t);
static dlg_ctor_fn      g_dlg_ctor_orig;
static volatile int     g_dlg_ctor_captures;
static void __attribute__((__thiscall__))
dlg_ctor_detour(void *self, uint32_t a1, uint32_t a2, uint32_t a3,
                uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7) {
    g_dlg_grid = (uint32_t)(uintptr_t)self;
    g_dlg_ctor_captures++;
    g_dlg_ctor_orig(self, a1, a2, a3, a4, a5, a6, a7);
}
static volatile int g_minhook_inited, g_dlg_hook_on;
static const char *dlg_hook_enable(int on) {
    if (on) {
        if (!g_minhook_inited) {
            if (MH_Initialize() != MH_OK) return "MH_Initialize failed";
            g_minhook_inited = 1;
        }
        if (g_dlg_hook_on) return "already-on";
        MH_STATUS s = MH_CreateHook((LPVOID)AP(VA_DLGGRID), (LPVOID)&dlg_ctor_detour,
                                    (LPVOID *)&g_dlg_ctor_orig);
        if (s != MH_OK && s != MH_ERROR_ALREADY_CREATED) return "MH_CreateHook failed";
        if (MH_EnableHook((LPVOID)AP(VA_DLGGRID)) != MH_OK) return "MH_EnableHook failed";
        g_dlg_hook_on = 1;
        return "enabled";
    }
    if (g_dlg_hook_on) { MH_DisableHook((LPVOID)AP(VA_DLGGRID)); g_dlg_hook_on = 0; }
    return "disabled";
}

static volatile LONG g_hooks_done;
static void ensure_hooks(void) {
    if (InterlockedCompareExchange(&g_hooks_done, 1, 0) != 0) return;   // install once
    void *base = VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!base) { vlog("[hooks] tramp alloc failed"); g_hooks_done = 0; return; }
    g_tramp_p = (uint8_t *)base;
    // Each poll hook's precb saves the poll esp+ecx(manager) into globals BEFORE pushad, so
    // the cb can read the poll's own `now` and inject into the live ring.  `mov [imm32],esp`
    // = 89 25 <addr>; `mov [imm32],ecx` = 89 0d <addr>.
    uint8_t pt[12] = { 0x89, 0x25, 0,0,0,0, 0x89, 0x0d, 0,0,0,0 };
    uint8_t pp[12] = { 0x89, 0x25, 0,0,0,0, 0x89, 0x0d, 0,0,0,0 };
    uint32_t a;
    a = (uint32_t)(uintptr_t)&g_ti_esp; memcpy(pt + 2, &a, 4);
    a = (uint32_t)(uintptr_t)&g_ti_mgr; memcpy(pt + 8, &a, 4);
    a = (uint32_t)(uintptr_t)&g_pk_esp; memcpy(pp + 2, &a, 4);
    a = (uint32_t)(uintptr_t)&g_pk_mgr; memcpy(pp + 8, &a, 4);
    // dialogue text-grid ctor (0x5e59c0, thiscall): precb saves ecx (the grid `this`) so we can
    // read/force the reveal state for ANY dialogue.  `mov [imm32],ecx` = 89 0d <addr>.
    uint8_t pg[6] = { 0x89, 0x0d, 0,0,0,0 };
    a = (uint32_t)(uintptr_t)&g_dlg_grid; memcpy(pg + 2, &a, 4);
    suspend_others(1);
    // The scene `this` for the (experimental) direct transition is read from 0x92dd4c, not a
    // hook — 0x581ba0 loops internally so its entry hook would not re-fire post-injection.
    install_detour((uintptr_t)AP(VA_INPUTPOLL), 5, (void *)poll_title_cb,  pt, 12);  // 53 8b 5c 24 0c
    install_detour((uintptr_t)AP(VA_MENUCTRL),  5, (void *)poll_picker_cb, pp, 12);  // 51 53 55 8b e9
    // NOTE: do NOT detour 0x5e59c0 (the grid ctor) — it is an SEH-scope function (installs an
    // exception frame in its prologue); the prologue-relocating detour breaks its unwind and the
    // newgame intro (which builds text grids + throws/catches internally) crashes.  g_dlg_grid
    // capture needs a non-SEH target (the per-tick stepper) or a pointer-chain instead. (void)pg;
    (void)pg;
    suspend_others(0);
    vlog("[hooks] installed: title 0x437c70, picker 0x4378d0, tramp=%p", base);
}

// ─── attract (title idle->demo) freeze ───────────────────────────────────────
static uint8_t     g_demo_orig[6];
static volatile int g_demo_saved;
static void attract_freeze(int on) {
    uint8_t *p = (uint8_t *)AP(VA_DEMO_JL);
    if (!mem_readable(p, 6)) return;
    // 0x583866  jl 0x5832e1  (0f 8c 75 fa ff ff)  ->  jmp 0x5832e1 + nop (rel is base-invariant)
    if (on) {
        if (!g_demo_saved) { memcpy(g_demo_orig, p, 6); g_demo_saved = 1; }
        uint8_t patch[6] = { 0xe9, 0x76, 0xfa, 0xff, 0xff, 0x90 };
        patch_bytes(p, patch, 6);
    } else if (g_demo_saved) {
        patch_bytes(p, g_demo_orig, 6);
    }
}

static volatile int g_autoskip;
// Story/cutscene/NPC dialogue AUTO-ADVANCE ("hold TAB" equivalent).  The story stepper (analog of
// base FUN_0043b980) gates its per-frame advance on the auto-advance flag [mgr+0x12c] read at
// 0x437738; the je@0x437740 skips the advance when that flag is 0.  NOP the je -> the advance
// (wrap cmd=8 -> reveal-to-end + step) fires EVERY frame -> the dialogue skips itself.  RE'd via a
// frida debugger (73x wrap(cmd=8), all gated here) + VERIFIED live (skips Mom's/Dad's/every box,
// hands-free, no stray world input).  Pure 2-byte code patch, no input injection.
static void dlg_autoskip(int on) {
    uint8_t *p = (uint8_t *)AP(VA_DLGADV_JE);
    if (!mem_readable(p, 2)) return;
    uint8_t nop2[2] = { 0x90, 0x90 };            // je 0x437752 -> nop nop : always advance
    uint8_t je2 [2] = { 0x74, 0x10 };            // restore (rel8 0x10 is base-invariant)
    patch_bytes(p, on ? nop2 : je2, 2);
    g_autoskip = on ? 1 : 0;
}

// ─── save-file inspection (reads savedataNN.sdt via the standalone sotes_save lib) ──
// The game keeps saves beside the exe in user\savedataNN.sdt.  Reading the FILE (not
// engine memory) means the trainer can enumerate/identify EVERY save without loading
// it — so it can pick an appropriate slot to `load`.  sotes_save is dependency-free
// (compiled into the DLL) and reusable outside the trainer (editor / the port).
static void trainer_save_path(int slot, char *out, int cap) {
    char mod[MAX_PATH]; mod[0] = 0;
    GetModuleFileNameA(NULL, mod, MAX_PATH); mod[MAX_PATH - 1] = 0;
    char *slash = strrchr(mod, '\\'); if (slash) *slash = 0;
    snprintf(out, cap, "%s\\user\\savedata%02d.sdt", mod, slot);
}
// Emit one slot's summary as JSON into out (always well-formed, even if absent/invalid).
static void save_json_one(int slot, char *out, int cap) {
    char path[512]; trainer_save_path(slot, path, sizeof path);
    sotes_save_info s;
    int rc = sotes_save_read(path, &s);
    if (rc == -2) { snprintf(out, cap, "{\"slot\":%d,\"present\":false}", slot); return; }
    if (!s.ok)    { snprintf(out, cap, "{\"slot\":%d,\"present\":true,\"valid\":false,\"error\":\"decode\"}", slot); return; }
    int n = snprintf(out, cap,
        "{\"slot\":%d,\"present\":true,\"valid\":%s,\"handle\":%u,\"file_size\":%u,"
        "\"key\":%u,\"checksum\":%u,\"party_count\":%d,\"party\":[",
        slot, s.valid ? "true" : "false", s.handle, (unsigned)s.file_size,
        s.hdr.key, s.checksum, s.party_count);
    for (int i = 0; i < s.party_count && n < cap; ++i)
        n += snprintf(out + n, cap - n, "%s{\"name\":\"%s\",\"code\":%u,\"level_base\":%d}",
                      i ? "," : "", s.party[i].name, s.party[i].code, s.party[i].level_base);
    if (n < cap) n += snprintf(out + n, cap - n, "],\"header_grid\":[");
    for (int k = 0; k < 16 && s.ph_present && n < cap; ++k)
        n += snprintf(out + n, cap - n, "%s%u", k ? "," : "", s.ph[k]);
    if (n < cap) snprintf(out + n, cap - n, "]}");
}

// ─── keep-active (no focus steal) + launcher dismiss + attract-off ───────────
// Three DEFAULT-ON boot behaviors so the trainer works while its own window is in the
// background (the normal case when an agent/UI drives it from another machine):
//   1. KEEP-ACTIVE — DirectDraw pauses the game loop on focus loss (retail halts input +
//      render on WM_ACTIVATEAPP deactivate, src wnd_proc).  We re-post WM_ACTIVATEAPP(TRUE)
//      to the main window (class CLASS_LIZSOFT_SOTES) each tick, so the game keeps running
//      UNFOCUSED — WITHOUT stealing focus (no SetForegroundWindow).  Toggle: `keepactive`.
//   2. LAUNCHER DISMISS — click through the #32770 launcher (Full/Safe/Wind/DB/Zoom) IN-
//      PROCESS (BM_CLICK works in-process, unlike an external click — quirk #3), so the
//      inject.exe ship path is hands-free.
//   3. ATTRACT OFF — freeze the title idle->demo trigger so the title never cycles to the
//      attract demo (which would break the menu-drive load).  Toggle: `attract`.
#define OSS_MAIN_WND_CLASS "CLASS_LIZSOFT_SOTES"
#define WM_ACTIVATEAPP_    0x001C
static volatile int g_launch_clicked;
static volatile int g_keepactive = 1;
static volatile int g_main_wnd_seen;   // the CLASS_LIZSOFT_SOTES window has appeared

static BOOL CALLBACK dismiss_child_cb(HWND hwnd, LPARAM lp) {
    (void)lp;
    char cls[64] = {0}, txt[128] = {0};
    GetClassNameA(hwnd, cls, sizeof cls);
    if (lstrcmpiA(cls, "Button") != 0) return TRUE;
    GetWindowTextA(hwnd, txt, sizeof txt);
    char t[128]; int i = 0;
    for (; txt[i] && i < (int)sizeof t - 1; ++i)
        t[i] = (char)((txt[i] >= 'A' && txt[i] <= 'Z') ? txt[i] + 32 : txt[i]);
    t[i] = 0;
    if (!g_launch_clicked && (strstr(t, "launch") || strstr(t, "start") ||
                              strstr(t, "play") || !strcmp(t, "ok") || !strcmp(t, "&ok"))) {
        PostMessageA(hwnd, BM_CLICK, 0, 0);
        g_launch_clicked = 1;
        vlog("[launcher] clicked '%s'", txt);
    }
    return TRUE;
}
static BOOL CALLBACK keepalive_top_cb(HWND hwnd, LPARAM lp) {
    (void)lp;
    DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    if (!g_launch_clicked) EnumChildWindows(hwnd, dismiss_child_cb, 0);   // launcher buttons
    char cls[64] = {0}; GetClassNameA(hwnd, cls, sizeof cls);
    if (lstrcmpiA(cls, OSS_MAIN_WND_CLASS) == 0) {
        g_main_wnd_seen = 1;
        g_game_hwnd = hwnd;                                            // for mouse-fly client->world
        if (g_keepactive) PostMessageA(hwnd, WM_ACTIVATEAPP_, 1, 0);   // keep active, no focus steal
    }
    return TRUE;
}
static void ensure_hooks(void);
static DWORD WINAPI keepalive_thread(void *unused) {
    (void)unused;
    attract_freeze(1);                                    // attract OFF by default
    dlg_autoskip(1);                                      // story-dialogue auto-skip ON by default (USER)
    ensure_hooks();                                       // install title/picker hooks early
    for (;;) { EnumWindows(keepalive_top_cb, 0); Sleep(50); }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Trainer core — the shared mechanics behind BOTH the socket commands and the
// ImGui UI (trainer_core.h).  Reads fill typed structs; actions mutate engine
// state.  Blocking drives (menu_load/newgame) are meant to run off the UI thread.
// ═══════════════════════════════════════════════════════════════════════════

// Write the player's authoritative phys-box (actor+0x40): box[+4]=X sticks, box[+8]=top
// gravity-settles.  Absolute y is a world_y (bottom) target -> top = y - height + 1.  Shared
// by the `teleport` command, tc_teleport, and mouse-fly.
static int teleport_box(int x_cpx, int y_cpx, int set_x, int set_y, int relative) {
    uintptr_t base = find_player();
    if (!base) return 0;
    uint32_t box = 0;
    if (!rd32((void *)(base + OFF_BOX), &box) || box <= 0x10000 ||
        !mem_readable((void *)(uintptr_t)box, 0x14)) return 0;
    if (set_x) {
        uint32_t cur = 0; rd32((void *)((uintptr_t)box + BOX_X), &cur);
        write_typed((uintptr_t)box + BOX_X, relative ? cur + (uint32_t)x_cpx : (uint32_t)x_cpx, "u32");
    }
    if (set_y) {
        uint32_t h = 0, curtop = 0;
        rd32((void *)((uintptr_t)box + BOX_H),   &h);
        rd32((void *)((uintptr_t)box + BOX_TOP), &curtop);
        uint32_t newtop = relative ? curtop + (uint32_t)y_cpx : (uint32_t)(y_cpx - (int)h + 1);
        write_typed((uintptr_t)box + BOX_TOP, newtop, "u32");
    }
    return 1;
}

// Resolve the camera object ptr *(render_root + g_cam_off).
static uint32_t read_cam(void) {
    uint32_t root = 0, cam = 0;
    if (!rd32(AP(VA_RENDER_ROOT), &root) || root <= 0x10000) return 0;
    if (!rd32((void *)(uintptr_t)(root + g_cam_off), &cam) || cam <= 0x10000) return 0;
    return cam;
}
// Resolve the world-view rect (top-left corner + span, all centi-px) from the camera object:
// left = cur_x (cam+0x60), top = cur_y (cam+0x5c) — the EASED scroll origin (NOT the instant
// player mirror); span from cam+0x64/+0x68 (so the resolution-enum zoom is handled).
static int read_view(int *left, int *top, int *vw, int *vh) {
    uint32_t cam = read_cam(), l = 0, t = 0, w = 0, h = 0;
    if (!cam) return 0;
    if (!rd32((void *)(uintptr_t)(cam + CAM_CUR_X), &l)) return 0;
    if (!rd32((void *)(uintptr_t)(cam + CAM_CUR_Y), &t)) return 0;
    rd32((void *)(uintptr_t)(cam + CAM_VP_W), &w);
    rd32((void *)(uintptr_t)(cam + CAM_VP_H), &h);
    if (w < 1000 || w > 4000000) w = 64000;   // fallback: 640px
    if (h < 1000 || h > 4000000) h = 48000;    // fallback: 480px
    *left = (int)l; *top = (int)t; *vw = (int)w; *vh = (int)h;
    return 1;
}
// Map a cursor at client (mx,my) over a cw x ch game window to world centi-px.
static int client_to_world(int mx, int my, int cw, int ch, int *wx_cpx, int *wy_cpx) {
    int left = 0, top = 0, vw = 0, vh = 0;
    if (cw <= 0 || ch <= 0 || !read_view(&left, &top, &vw, &vh)) return 0;
    *wx_cpx = left + (int)((double)mx / cw * vw);
    *wy_cpx = top  + (int)((double)my / ch * vh);
    return 1;
}

// ── portal hijack: overwrite a door's target_room in the LIVE room record (SE_CODE_MAP "WARP
// PRIMITIVE") so using that door warps to the chosen room.  Stash the original per (room_key,
// slot) so revert restores it even after the room-record ptr moves.  Within-area only (a
// cross-AREA target crashes at 0x5ad656 until its W-map is resident — SE_CODE_MAP).
#define MAX_HIJACK 32
static struct { int used; uint32_t room_key; int slot; uint32_t orig; } g_hijacks[MAX_HIJACK];
static CRITICAL_SECTION g_hj_cs;
static uint32_t cur_room_record(uint32_t *room_key) {
    uint32_t root = 0, rr = 0, key = 0;
    if (!rd32(AP(VA_RENDER_ROOT), &root) || root <= 0x10000) return 0;
    if (!rd32((void *)(uintptr_t)(root + ROOT_ROOM_REC), &rr) || rr <= 0x10000 ||
        !mem_readable((void *)(uintptr_t)rr, RR_SIZE)) return 0;
    rd32((void *)(uintptr_t)(rr + RR_ROOM_KEY), &key);
    if (room_key) *room_key = key;
    return rr;
}
static uint32_t exit_target_addr(uint32_t rr, int slot) {
    return rr + RR_EXIT0 + (uint32_t)slot * RR_EXIT_STRIDE + RR_EXIT_TARGET;
}
static void hijack_exit_engine(int slot, uint32_t target) {
    if (slot < 0 || slot >= 20) return;
    uint32_t key = 0, rr = cur_room_record(&key);
    if (!rr) return;
    uint32_t addr = exit_target_addr(rr, slot);
    if (!mem_writable((void *)(uintptr_t)addr, 4)) return;
    uint32_t orig = 0; rd32((void *)(uintptr_t)addr, &orig);
    EnterCriticalSection(&g_hj_cs);
    int freei = -1, found = 0;
    for (int i = 0; i < MAX_HIJACK; ++i) {
        if (g_hijacks[i].used && g_hijacks[i].room_key == key && g_hijacks[i].slot == slot) { found = 1; break; }
        if (!g_hijacks[i].used && freei < 0) freei = i;
    }
    if (!found && freei >= 0) {   // first hijack of this slot -> remember the original target
        g_hijacks[freei].used = 1; g_hijacks[freei].room_key = key;
        g_hijacks[freei].slot = slot; g_hijacks[freei].orig = orig;
    }
    LeaveCriticalSection(&g_hj_cs);
    *(volatile uint32_t *)(uintptr_t)addr = target;
}
static void revert_exit_engine(int slot) {
    if (slot < 0 || slot >= 20) return;
    uint32_t key = 0, rr = cur_room_record(&key);
    if (!rr) return;
    EnterCriticalSection(&g_hj_cs);
    for (int i = 0; i < MAX_HIJACK; ++i)
        if (g_hijacks[i].used && g_hijacks[i].room_key == key && g_hijacks[i].slot == slot) {
            uint32_t addr = exit_target_addr(rr, slot);
            if (mem_writable((void *)(uintptr_t)addr, 4)) *(volatile uint32_t *)(uintptr_t)addr = g_hijacks[i].orig;
            g_hijacks[i].used = 0;
            break;
        }
    LeaveCriticalSection(&g_hj_cs);
}
static int exit_hijacked(uint32_t room_key, int slot, uint32_t *orig) {
    int r = 0;
    EnterCriticalSection(&g_hj_cs);
    for (int i = 0; i < MAX_HIJACK; ++i)
        if (g_hijacks[i].used && g_hijacks[i].room_key == room_key && g_hijacks[i].slot == slot) {
            if (orig) *orig = g_hijacks[i].orig;
            r = 1; break;
        }
    LeaveCriticalSection(&g_hj_cs);
    return r;
}
// Engine-side stat write (statblock resolved on the engine thread; optional freeze lock).
static void setstat_engine(int off, uint32_t val, int lock) {
    uintptr_t base = find_player(); uint32_t sb = 0;
    if (off < 0 || !base || !rd32((void *)(base + OFF_STATBLOCK), &sb)) return;
    write_typed(sb + off, val, "u32");
    if (lock) lock_set(sb + off, val);
}

// ═══════════════════════════════════════════════════════════════════════════
// Engine-thread execution queue (USER: poke/call safely on the game's own thread).
// Pokes + engine-fn CALLS are unsafe from a worker thread — the engine may be
// mid-realloc/teardown (a call into non-reentrant engine code, or a write to
// just-freed memory, crashes).  So worker threads (socket/UI) ENQUEUE jobs and the
// inputPoll safepoint (poll_title_cb — engine thread, fires every frame at the title
// AND in freeroam) DRAINS + runs them in-context.  Reads stay off-thread (guarded,
// non-mutating).  The per-frame freezes + mouse-fly ALSO run in the drain, so NO
// worker thread ever writes engine memory or calls engine code.
// ═══════════════════════════════════════════════════════════════════════════
enum { EQ_CALL = 1, EQ_WRITE, EQ_TELEPORT, EQ_SETSTAT, EQ_HIJACK, EQ_REVERT };
// state: 0 free, 1 pending, 3 running, 2 done.  Payload is COPIED into the slot, so a timed-out
// waiter can never leave the engine thread dereferencing a dead caller stack.
// u[]: EQ_CALL {0=fn,1=ecx,2=argc,3..10=args}; EQ_WRITE {0=addr,1=val,2=width}; EQ_TELEPORT
// {0=x,1=y,2=set_x,3=set_y,4=rel}; EQ_SETSTAT {0=off,1=val,2=lock}; EQ_HIJACK {0=slot,1=target};
// EQ_REVERT {0=slot}.
typedef struct { volatile int state; int kind; uint32_t u[12]; uint32_t ret; } eq_job;
#define EQ_N 32
static eq_job g_eq[EQ_N];
static CRITICAL_SECTION g_eq_cs;
static eq_job *eq_alloc(int kind) {
    EnterCriticalSection(&g_eq_cs);
    for (int i = 0; i < EQ_N; ++i)
        if (g_eq[i].state == 0) {
            memset(&g_eq[i], 0, sizeof g_eq[i]);
            g_eq[i].kind = kind; g_eq[i].state = 1;
            LeaveCriticalSection(&g_eq_cs);
            return &g_eq[i];
        }
    LeaveCriticalSection(&g_eq_cs);
    return NULL;   // queue full (worker will just drop the action)
}
// Block up to wait_ms for the engine thread to run j; return j->ret (0 on cancel/timeout).  Frees j.
static uint32_t engine_wait(eq_job *j, int wait_ms) {
    if (!j) return 0;
    for (int t = 0; ; ++t) {
        if (j->state == 2) { uint32_t r = j->ret; j->state = 0; return r; }
        if (wait_ms > 0 && t >= wait_ms) {
            EnterCriticalSection(&g_eq_cs);
            if (j->state == 1) { j->state = 0; LeaveCriticalSection(&g_eq_cs); return 0; }  // cancel before run
            LeaveCriticalSection(&g_eq_cs);   // else running(3): keep waiting (payload is queue-owned)
        }
        Sleep(1);
    }
}
// Execute one job in-context (engine thread) — the ONLY place engine memory is poked / engine
// code is called from the trainer.
static void eq_exec(eq_job *j) {
    switch (j->kind) {
    case EQ_CALL:     j->ret = call_va((void *)(uintptr_t)j->u[0], j->u[1], &j->u[3], (int)j->u[2]); break;
    case EQ_WRITE:    write_typed((uintptr_t)j->u[0], j->u[1], j->u[2] == 1 ? "u8" : j->u[2] == 2 ? "u16" : "u32"); break;
    case EQ_TELEPORT: j->ret = (uint32_t)teleport_box((int)j->u[0], (int)j->u[1], (int)j->u[2], (int)j->u[3], (int)j->u[4]); break;
    case EQ_SETSTAT:  setstat_engine((int)j->u[0], j->u[1], (int)j->u[2]); break;
    case EQ_HIJACK:   hijack_exit_engine((int)j->u[0], j->u[1]); break;
    case EQ_REVERT:   revert_exit_engine((int)j->u[0]); break;
    default: break;
    }
}
static void drain_engine_queue(void) {
    for (int i = 0; i < EQ_N; ++i) {
        int run = 0;
        EnterCriticalSection(&g_eq_cs);
        if (g_eq[i].state == 1) { g_eq[i].state = 3; run = 1; }
        LeaveCriticalSection(&g_eq_cs);
        if (run) { eq_exec(&g_eq[i]); g_eq[i].state = 2; }
    }
}

// Per-frame engine-side work (run by the drain, so it happens IN the game loop, engine-side):
//   freezes  — god (hp+mp = GOD_VAL 9999) / setstat locks / fastskip reveal.
//   mousefly — teleport the player to the cursor over the game window (camera frozen).
//   hotkeys  — F7 toggles mouse-fly.
static void engine_freezes(void) {
    if (g_god) {   // god = invincibility: pin hp+mp above max so nothing kills + free casting
        uintptr_t base = find_player(); uint32_t sb = 0;
        if (base && rd32((void *)(base + OFF_STATBLOCK), &sb)) {
            if (mem_writable((void *)(sb + OFF_HP_CUR), 4)) *(uint32_t *)(sb + OFF_HP_CUR) = GOD_VAL;
            if (mem_writable((void *)(sb + OFF_MP_CUR), 4)) *(uint32_t *)(sb + OFF_MP_CUR) = GOD_VAL;
        }
    }
    EnterCriticalSection(&g_lock_cs);
    for (int i = 0; i < MAX_LOCKS; ++i)
        if (g_locks[i].used && mem_writable((void *)g_locks[i].addr, 4)) *(uint32_t *)g_locks[i].addr = g_locks[i].val;
    LeaveCriticalSection(&g_lock_cs);
    if (g_fastskip) {
        if (!g_dlg_grid) {
            static int rescan_div = 0;
            if (++rescan_div >= 8) { rescan_div = 0; uintptr_t hit[1]; if (find_dlg_grids(hit, 1) == 1) g_dlg_grid = (uint32_t)hit[0]; }
        }
        dlg_force_reveal(g_dlg_grid);
    }
}
// Continuously teleport the player to the cursor over the game window.  The camera-follow would
// otherwise scroll the view out from under the cursor (positive feedback -> runaway to the map
// edge), so while flying we FREEZE the view: latch cur_x/cur_y at fly-start, pin cur+tgt to the
// latch each frame, and map the cursor against the LATCH (a stable world rect) — she goes exactly
// under the cursor and stays.  To still fly ACROSS the map, EDGE-SCROLL: when the cursor nears a
// screen edge, pan the frozen latch that way (clamped to [0, map - viewport]), so the view follows
// her toward the edges.  The view resumes normal follow when fly is turned off.
static void engine_mousefly(void) {
    if (!g_mousefly || !g_game_hwnd) { g_fly_latched = 0; return; }
    POINT p; RECT rc;
    if (!(GetCursorPos(&p) && GetClientRect(g_game_hwnd, &rc) && ScreenToClient(g_game_hwnd, &p))) return;
    int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
    if (cw <= 0 || ch <= 0) return;
    uint32_t cam = read_cam(), t = 0;
    int vw = 64000, vh = 48000, mw = 0, mh = 0;
    if (cam) {
        if (rd32((void *)(uintptr_t)(cam + CAM_VP_W), &t) && t > 1000 && t < 4000000) vw = (int)t;
        if (rd32((void *)(uintptr_t)(cam + CAM_VP_H), &t) && t > 1000 && t < 4000000) vh = (int)t;
        rd32((void *)(uintptr_t)(cam + CAM_MAP_W), &t); mw = (int)t;
        rd32((void *)(uintptr_t)(cam + CAM_MAP_H), &t); mh = (int)t;
        if (!g_fly_latched) {   // rising edge: latch the current eased scroll origin (view top-left)
            uint32_t l = 0, tt = 0;
            rd32((void *)(uintptr_t)(cam + CAM_CUR_X), &l);
            rd32((void *)(uintptr_t)(cam + CAM_CUR_Y), &tt);
            g_fly_fx = (int)l; g_fly_fy = (int)tt; g_fly_latched = 1;
        }
    }
    if (!g_fly_latched) return;
    // edge-scroll the frozen latch when the cursor nears an edge, RAMPED by how deep into the zone
    // the cursor is (0 at the zone boundary -> panmax at the very edge), clamped to the scroll range
    int emx = cw / 5, emy = ch / 5, panmax = 360;   // outer 20% is the pan zone; ~3.6px/frame max
    if      (p.x < emx)      g_fly_fx -= panmax * (emx - p.x) / (emx ? emx : 1);
    else if (p.x > cw - emx) g_fly_fx += panmax * (p.x - (cw - emx)) / (emx ? emx : 1);
    if      (p.y < emy)      g_fly_fy -= panmax * (emy - p.y) / (emy ? emy : 1);
    else if (p.y > ch - emy) g_fly_fy += panmax * (p.y - (ch - emy)) / (emy ? emy : 1);
    if (mw > 0 && mh > 0) {   // clamp to [0, map - viewport] (only when the map dims read sane)
        int maxx = mw > vw ? mw - vw : 0, maxy = mh > vh ? mh - vh : 0;
        if (g_fly_fx < 0) g_fly_fx = 0; else if (g_fly_fx > maxx) g_fly_fx = maxx;
        if (g_fly_fy < 0) g_fly_fy = 0; else if (g_fly_fy > maxy) g_fly_fy = maxy;
    }
    // freeze the view at the (possibly panned) latch: pin cur + tgt so the easer parks it there
    if (cam) {
        if (mem_writable((void *)(uintptr_t)(cam + CAM_CUR_X), 4)) *(uint32_t *)(uintptr_t)(cam + CAM_CUR_X) = (uint32_t)g_fly_fx;
        if (mem_writable((void *)(uintptr_t)(cam + CAM_CUR_Y), 4)) *(uint32_t *)(uintptr_t)(cam + CAM_CUR_Y) = (uint32_t)g_fly_fy;
        if (mem_writable((void *)(uintptr_t)(cam + CAM_TGT_X), 4)) *(uint32_t *)(uintptr_t)(cam + CAM_TGT_X) = (uint32_t)g_fly_fx;
        if (mem_writable((void *)(uintptr_t)(cam + CAM_TGT_Y), 4)) *(uint32_t *)(uintptr_t)(cam + CAM_TGT_Y) = (uint32_t)g_fly_fy;
    }
    // map the cursor against the frozen latch and place her there
    if (p.x >= 0 && p.y >= 0 && p.x < cw && p.y < ch) {
        int wx = g_fly_fx + (int)((double)p.x / cw * vw);
        int wy = g_fly_fy + (int)((double)p.y / ch * vh);
        teleport_box(wx, wy, 1, 1, 0);
    }
}
static void engine_hotkeys(void) {
    static int prev_f7 = 0;
    int f7 = (GetAsyncKeyState(VK_F7) & 0x8000) ? 1 : 0;
    if (f7 && !prev_f7) g_mousefly = !g_mousefly;
    prev_f7 = f7;
}

// ── the room-record TABLE walk (SE_CODE_MAP thread #3): the current room record is one entry
// in a contiguous 0x150-B table of every room.  Seed from it, walk back to the table base, then
// forward — the (room_key,area,scene) signature bounds each valid record.  Feeds the portal
// destination fuzzy-search (valid target room keys).
static int room_rec_valid(uintptr_t rr) {
    if (!mem_readable((void *)rr, RR_SIZE)) return 0;
    uint32_t key = 0, area = 0, scene = 0;
    rd32((void *)(rr + RR_ROOM_KEY), &key);
    rd32((void *)(rr + RR_AREA),     &area);
    rd32((void *)(rr + RR_SCENE),    &scene);
    if (key < 0x1000 || key > 0x7fffffff) return 0;
    if (area < 1 || area > 4095)          return 0;
    if (scene < 1 || scene > 19999)       return 0;
    return 1;
}
int tc_get_rooms(tc_room *out, int cap) {
    if (!out || cap <= 0) return 0;
    uint32_t rr0 = cur_room_record(NULL);
    if (!rr0 || !room_rec_valid(rr0)) return 0;
    uintptr_t base = rr0;
    for (int i = 0; i < 8192; ++i) { uintptr_t p = base - RR_SIZE; if (!room_rec_valid(p)) break; base = p; }
    int n = 0;
    for (uintptr_t p = base; n < cap; p += RR_SIZE) {
        if (!room_rec_valid(p)) break;
        uint32_t key = 0, area = 0, scene = 0;
        rd32((void *)(p + RR_ROOM_KEY), &key);
        rd32((void *)(p + RR_AREA),     &area);
        rd32((void *)(p + RR_SCENE),    &scene);
        out[n].key = key; out[n].area = area; out[n].scene = scene; ++n;
    }
    return n;
}

// ── typed reads for the UI (trainer_core.h) ──
int tc_get_player(tc_player *o) {
    if (!o) return 0;
    memset(o, 0, sizeof *o);
    uintptr_t base = find_player();
    if (!base) return 0;
    uint32_t wx = 0, wy = 0, sb = 0, hp = 0, mp = 0, lvl = 0, ec = 0, em = 0;
    rd32((void *)(base + OFF_WORLD_X), &wx);   rd32((void *)(base + OFF_WORLD_Y), &wy);
    rd32((void *)(base + OFF_STATBLOCK), &sb);
    rd32((void *)(sb + OFF_HP_CUR), &hp);      rd32((void *)(sb + OFF_MP_CUR), &mp);
    rd32((void *)(sb + OFF_LEVEL), &lvl);      rd32((void *)(sb + OFF_EXP_CUR), &ec);
    rd32((void *)(sb + OFF_EXP_MAX), &em);
    o->ok = 1; o->actor = (uint32_t)base; o->stat_block = sb;
    o->world_x = (int)wx; o->world_y = (int)wy;
    o->hp = (int)hp; o->hp_max = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    o->mp = (int)mp; o->mp_max = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    o->level_base = (int)lvl; o->exp_cur = (int)ec; o->exp_max = (int)em;
    return 1;
}
int tc_get_map(tc_map *o) {
    if (!o) return 0;
    memset(o, 0, sizeof *o);
    uint32_t key = 0, rr = cur_room_record(&key);
    if (!rr) return 0;
    uint32_t root = 0; rd32(AP(VA_RENDER_ROOT), &root);
    o->ok = 1; o->render_root = root; o->room_record = rr; o->room_key = key;
    rd32((void *)(uintptr_t)(rr + RR_AREA),     &o->area);
    rd32((void *)(uintptr_t)(rr + RR_SCENE),    &o->scene);
    rd32((void *)(uintptr_t)(rr + RR_TILESET),  &o->tileset);
    rd32((void *)(uintptr_t)(rr + RR_PARALLAX), &o->parallax);
    int n = 0;
    for (int k = 0; k < 20; ++k) {
        uintptr_t b = rr + RR_EXIT0 + (uint32_t)k * RR_EXIT_STRIDE;
        uint32_t ek = 0, tr = 0, ret = 0;
        rd32((void *)b, &ek); rd32((void *)(b + 4), &tr); rd32((void *)(b + 8), &ret);
        if (ek == 0 && tr == 0) continue;
        tc_exit *e = &o->exits[n++];
        e->exit_key = ek; e->target_room = tr; e->return_key = ret; e->slot = k;
        e->hijacked = exit_hijacked(key, k, &e->orig_target);
    }
    o->n_exits = n;
    return 1;
}
int tc_get_saves(tc_save *o, int cap) {
    if (!o || cap <= 0) return 0;
    int n = 0;
    for (int slot = 0; slot < 16 && n < cap; ++slot) {
        char path[512]; trainer_save_path(slot, path, sizeof path);
        sotes_save_info s;
        if (sotes_save_read(path, &s) == -2) continue;   // absent
        tc_save *d = &o[n++];
        memset(d, 0, sizeof *d);
        d->slot = slot; d->present = 1; d->valid = s.ok && s.valid;
        if (s.ok) {
            d->handle = s.handle; d->file_size = s.file_size; d->party_count = s.party_count;
            if (s.party_count > 0) {
                snprintf(d->party0, sizeof d->party0, "%s", s.party[0].name);
                d->level0 = s.party[0].level_base;
            }
        }
    }
    return n;
}
int tc_get_status(tc_status *o) {
    if (!o) return 0;
    memset(o, 0, sizeof *o);
    o->hooks = (int)g_hooks_done;
    o->player_present = find_player() ? 1 : 0;
    o->at_title = (g_ti_mgr && !o->player_present) ? 1 : 0;
    o->autoskip = g_autoskip; o->mousefly = g_mousefly;
    o->dlgskip = g_dlgskip; o->god = g_god; o->keepactive = g_keepactive;
    o->attract = g_demo_saved; o->fastskip = g_fastskip;
    o->delta = (uint32_t)g_delta; o->base = (uint32_t)(ORIG_BASE + g_delta);
    o->ti_mgr = g_ti_mgr; o->pk_mgr = g_pk_mgr; o->game_hwnd = (uint32_t)(uintptr_t)g_game_hwnd;
    int vw = 0, vh = 0;
    o->cam_ok = read_view(&o->cam_x, &o->cam_y, &vw, &vh);   // cam_x/y = the view top-left corner
    return 1;
}

// ── actions for the UI (all mutation is ENQUEUED onto the engine thread) ──
void tc_teleport(int x, int y, int set_x, int set_y, int relative) {
    eq_job *j = eq_alloc(EQ_TELEPORT); if (!j) return;
    j->u[0] = (uint32_t)x; j->u[1] = (uint32_t)y;
    j->u[2] = (uint32_t)set_x; j->u[3] = (uint32_t)set_y; j->u[4] = (uint32_t)relative;
    engine_wait(j, 200);
}
void tc_set_toggle(const char *name, int on) {
    if (!name) return;
    if      (!strcmp(name, "autoskip"))   dlg_autoskip(on);
    else if (!strcmp(name, "mousefly"))   g_mousefly = on ? 1 : 0;
    else if (!strcmp(name, "dlgskip"))    g_dlgskip = on ? 1 : 0;
    else if (!strcmp(name, "god"))        g_god = on ? 1 : 0;
    else if (!strcmp(name, "keepactive")) g_keepactive = on ? 1 : 0;
    else if (!strcmp(name, "attract"))    attract_freeze(on);
    else if (!strcmp(name, "fastskip"))   g_fastskip = on ? 1 : 0;
}
int tc_get_toggle(const char *name) {
    if (!name) return 0;
    if      (!strcmp(name, "autoskip"))   return g_autoskip;
    else if (!strcmp(name, "mousefly"))   return g_mousefly;
    else if (!strcmp(name, "dlgskip"))    return g_dlgskip;
    else if (!strcmp(name, "god"))        return g_god;
    else if (!strcmp(name, "keepactive")) return g_keepactive;
    else if (!strcmp(name, "attract"))    return g_demo_saved;
    else if (!strcmp(name, "fastskip"))   return g_fastskip;
    return 0;
}
void tc_setstat(const char *which, int value, int lock) {
    int off = -1;
    if      (which && !strcmp(which, "hp"))     off = OFF_HP_CUR;
    else if (which && !strcmp(which, "hp_max")) off = OFF_HP_BASE;
    else if (which && !strcmp(which, "mp"))     off = OFF_MP_CUR;
    else if (which && !strcmp(which, "mp_max")) off = OFF_MP_BASE;
    else if (which && !strcmp(which, "level"))  off = OFF_LEVEL;
    if (off < 0) return;
    eq_job *j = eq_alloc(EQ_SETSTAT); if (!j) return;
    j->u[0] = (uint32_t)off; j->u[1] = (uint32_t)value; j->u[2] = (uint32_t)lock;
    engine_wait(j, 200);
}
void tc_hijack_exit(int slot, uint32_t target) {
    eq_job *j = eq_alloc(EQ_HIJACK); if (!j) return;
    j->u[0] = (uint32_t)slot; j->u[1] = target;
    engine_wait(j, 200);
}
void tc_revert_exit(int slot) {
    eq_job *j = eq_alloc(EQ_REVERT); if (!j) return;
    j->u[0] = (uint32_t)slot;
    engine_wait(j, 200);
}
void tc_set_cam_off(uint32_t off) { g_cam_off = off; }
uint32_t tc_get_cam_off(void)     { return g_cam_off; }

// The VERIFIED menu-drive load/newgame (see the `load`/`newgame` commands).  Canonical impls;
// both the socket commands and the UI call these.  BLOCKING — run off the UI render thread.
static int menu_load(int slot, int downs, char *msg, int cap) {
    ensure_hooks(); attract_freeze(1);                   // hold the title (no demo mid-drive)
    g_md_slot = slot; g_md_downs = downs;
    g_pk_diag[0] = 0; g_pk_diag_done = 0;
    g_pk_mgr = 0;                                         // clear so poll_title_cb sees the picker OPEN
    g_md_state = 1;                                       // arm: title-confirm -> picker-confirm
    int loaded = 0;
    for (int i = 0; i < 1200; ++i) { if (find_player()) { loaded = 1; break; } Sleep(10); }
    g_md_state = 0; g_md_slot = -1;
    if (msg) snprintf(msg, cap, "%s", g_pk_diag);
    return loaded;
}
static int menu_newgame(int to, int btn, int gap) {
    ensure_hooks(); attract_freeze(1);
    g_ng_rotates = to; g_ng_btn = btn; g_ng_gap = gap; g_ng_cool = 4;
    g_ng_state = 1;
    int started = 0;
    for (int i = 0; i < 3000; ++i) { if (find_player()) { started = 1; break; } Sleep(10); }
    g_ng_state = 0;
    return started;
}
int tc_load(int slot, char *msg, int cap) { return menu_load(slot, 0, msg, cap); }
int tc_newgame(void)                       { return menu_newgame(1, 0x04, 6); }

// ─── command dispatch ───────────────────────────────────────────────────────
static void handle_line(const char *line, char *out, int cap) {
    char cmd[32] = {0};
    if (!json_str(line, "cmd", cmd, sizeof cmd)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no cmd\"}"); return; }

    if (!strcmp(cmd, "ping")) {
        snprintf(out, cap, "{\"ok\":true,\"pong\":true,\"delta\":\"0x%x\",\"base\":\"0x%x\"}",
                 (unsigned)g_delta, (unsigned)(ORIG_BASE + g_delta));
        return;
    }
    if (!strcmp(cmd, "player")) {
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "state")) {
        // diagnostic: boot/hook/menu-drive state.  ti_mgr!=0 => the TITLE input poll
        // (0x437c70) is firing (game is at the title); pk_mgr!=0 => the save-slot picker
        // poll (0x4378d0) is firing.  box_open/box_scale = the live dialogue-widget read
        // dlgskip gates on (box_open true ⇒ a dialogue box is on screen).
        uint32_t bsc = 0;
        if (g_ti_mgr) rd32((void *)(uintptr_t)(g_ti_mgr + DLG_BOX_SCALE), &bsc);
        snprintf(out, cap,
            "{\"ok\":true,\"hooks\":%d,\"main_wnd\":%s,\"launch_clicked\":%d,"
            "\"attract_frozen\":%s,\"keepactive\":%s,\"dlgskip\":%s,\"autoskip\":%s,\"fastskip\":%s,"
            "\"god\":%s,\"mousefly\":%s,\"box_open\":%s,\"box_scale\":%u,"
            "\"md_state\":%d,\"md_slot\":%d,"
            "\"ng_state\":%d,\"ti_mgr\":\"0x%08x\",\"pk_mgr\":\"0x%08x\"}",
            (int)g_hooks_done, g_main_wnd_seen ? "true" : "false", g_launch_clicked,
            g_demo_saved ? "true" : "false", g_keepactive ? "true" : "false",
            g_dlgskip ? "true" : "false", g_autoskip ? "true" : "false", g_fastskip ? "true" : "false",
            g_god ? "true" : "false", g_mousefly ? "true" : "false",
            dialogue_box_open(g_ti_mgr) ? "true" : "false",
            (unsigned)bsc, g_md_state, g_md_slot,
            g_ng_state, (unsigned)g_ti_mgr, (unsigned)g_pk_mgr);
        return;
    }
    if (!strcmp(cmd, "read")) {
        long long a; char ty[8] = "u32";
        if (!json_num(line, "addr", &a)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no addr\"}"); return; }
        json_str(line, "type", ty, sizeof ty);
        int reloc = json_bool(line, "va", 0);           // va:true -> AP() relocation
        uintptr_t addr = reloc ? (uintptr_t)AP(a) : (uintptr_t)a;
        snprintf(out, cap, "{\"ok\":true,\"value\":%u,\"addr\":\"0x%08x\"}",
                 read_typed(addr, ty), (unsigned)addr);
        return;
    }
    if (!strcmp(cmd, "write")) {
        long long a, v; char ty[8] = "u32";
        if (!json_num(line, "addr", &a) || !json_num(line, "value", &v)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"addr/value\"}"); return; }
        json_str(line, "type", ty, sizeof ty);
        int reloc = json_bool(line, "va", 0);
        uintptr_t addr = reloc ? (uintptr_t)AP(a) : (uintptr_t)a;
        int width = !strcmp(ty, "u8") ? 1 : !strcmp(ty, "u16") ? 2 : 4;
        eq_job *j = eq_alloc(EQ_WRITE);                  // poke on the engine thread (safepoint)
        if (j) { j->u[0] = (uint32_t)addr; j->u[1] = (uint32_t)v; j->u[2] = (uint32_t)width; engine_wait(j, 200); }
        snprintf(out, cap, "{\"ok\":%s,\"wrote\":%u,\"addr\":\"0x%08x\"}", j ? "true" : "false", (uint32_t)v, (unsigned)addr);
        return;
    }
    if (!strcmp(cmd, "god")) {
        // {"cmd":"god","on":true|false} — freeze hp+mp at 9999 each frame (invincible + free casting).
        // DEFAULT ON.
        g_god = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"god\":%s}", g_god ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "mousefly")) {
        // {"cmd":"mousefly","on":true|false} — continuously teleport the player to the cursor over
        // the game window (also toggled by F7).  Uses the camera + a 640x480 view to map client->world.
        g_mousefly = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"mousefly\":%s}", g_mousefly ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "flyto")) {
        // {"cmd":"flyto","mx":X,"my":Y} — map a GIVEN game-window client point to world + teleport
        // there (the mouse-fly mapping, but for a specified point — validates the calibration without
        // the live cursor).  Reports the client rect + the resolved world target.
        long long mx, my;
        if (!json_num(line, "mx", &mx) || !json_num(line, "my", &my)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"mx/my\"}"); return; }
        RECT rc; int cw = 0, ch = 0;
        if (g_game_hwnd && GetClientRect(g_game_hwnd, &rc)) { cw = rc.right - rc.left; ch = rc.bottom - rc.top; }
        int wx = 0, wy = 0, ok = client_to_world((int)mx, (int)my, cw, ch, &wx, &wy);
        if (ok) tc_teleport(wx, wy, 1, 1, 0);
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":%s,\"client_w\":%d,\"client_h\":%d,\"world_x\":%d,\"world_y\":%d,\"player\":%s}",
                 ok ? "true" : "false", cw, ch, wx, wy, pj);
        return;
    }
    if (!strcmp(cmd, "setstat")) {
        char which[8] = {0}; long long v;
        if (!json_str(line, "which", which, sizeof which) || !json_num(line, "value", &v)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"which/value\"}"); return; }
        if (!find_player()) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no player\"}"); return; }
        tc_setstat(which, (int)v, json_bool(line, "lock", 0));   // resolves+writes on the engine thread
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "teleport")) {
        // Write the AUTHORITATIVE phys-box (actor+0x40 -> box), engine-side (enqueued).  box[+4]=X
        // sticks; box[+8]=top settles via gravity.  Absolute x/y = world centi-px; relative = px.
        long long x, y; int rel = json_bool(line, "relative", 0);
        int hasx = json_num(line, "x", &x), hasy = json_num(line, "y", &y);
        if (!hasx && !hasy) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no x/y\"}"); return; }
        if (!find_player()) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no player\"}"); return; }
        int xc = rel ? (int)(x * 100) : (int)x;
        int yc = rel ? (int)(y * 100) : (int)y;
        tc_teleport(xc, yc, hasx, hasy, rel);
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "box")) {                    // debug: read the player's phys-box
        uintptr_t base = find_player(); uint32_t box = 0;
        if (!base || !rd32((void *)(base + OFF_BOX), &box) || box <= 0x10000) {
            snprintf(out, cap, "{\"ok\":false,\"error\":\"no box\"}"); return; }
        uint32_t bx=0,top=0,w=0,h=0,tag=0;
        rd32((void*)((uintptr_t)box+0),&tag);   rd32((void*)((uintptr_t)box+BOX_X),&bx);
        rd32((void*)((uintptr_t)box+BOX_TOP),&top); rd32((void*)((uintptr_t)box+BOX_W),&w);
        rd32((void*)((uintptr_t)box+BOX_H),&h);
        snprintf(out, cap, "{\"ok\":true,\"box\":\"0x%08x\",\"tag\":\"0x%08x\",\"x\":%d,"
                 "\"top\":%d,\"w\":%d,\"h\":%d,\"world_y\":%d}",
                 box, tag, (int)bx, (int)top, (int)w, (int)h, (int)top+(int)h-1);
        return;
    }
    if (!strcmp(cmd, "map")) {
        // {"cmd":"map"} — the CURRENT map/room: read the render-root chain (*0x92dd38 ->
        // +0x1038 room record) and report room_key / area / DATA-scene id / tileset / parallax
        // + the EXIT GRAPH (portals -> target rooms).  Pure read; RE'd this session (SE_CODE_MAP).
        uint32_t root = 0, rr = 0;
        if (!rd32(AP(VA_RENDER_ROOT), &root) || root <= 0x10000 ||
            !rd32((void *)(uintptr_t)(root + ROOT_ROOM_REC), &rr) || rr <= 0x10000 ||
            !mem_readable((void *)(uintptr_t)rr, 0x150)) {
            snprintf(out, cap, "{\"ok\":false,\"error\":\"no room record (not in a scene?)\"}"); return; }
        uint32_t key=0, area=0, scene=0, tset=0, plx=0;
        rd32((void *)(uintptr_t)(rr + RR_ROOM_KEY), &key);
        rd32((void *)(uintptr_t)(rr + RR_AREA),     &area);
        rd32((void *)(uintptr_t)(rr + RR_SCENE),    &scene);
        rd32((void *)(uintptr_t)(rr + RR_TILESET),  &tset);
        rd32((void *)(uintptr_t)(rr + RR_PARALLAX), &plx);
        int n = snprintf(out, cap,
            "{\"ok\":true,\"render_root\":\"0x%08x\",\"room_record\":\"0x%08x\",\"room_key\":%u,"
            "\"area\":%u,\"scene\":%u,\"tileset\":%u,\"parallax\":%u,\"exits\":[",
            root, rr, key, area, scene, tset, plx);
        int first = 1;
        for (int k = 0; k < 20 && n < cap - 96; ++k) {
            uintptr_t b = rr + RR_EXIT0 + (uint32_t)k * 12;
            uint32_t ek=0, tr=0, ret=0;
            rd32((void *)b, &ek); rd32((void *)(b + 4), &tr); rd32((void *)(b + 8), &ret);
            if (ek == 0 && tr == 0) continue;
            n += snprintf(out + n, cap - n,
                "%s{\"exit_key\":%u,\"target_room\":%u,\"return_key\":%u}", first ? "" : ",", ek, tr, ret);
            first = 0;
        }
        snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "hijack")) {
        // {"cmd":"hijack","slot":N,"target":ROOMKEY} — overwrite exit-slot N's target_room in the
        // live room record so that door warps to ROOMKEY (SE_CODE_MAP WARP PRIMITIVE).  Within-area.
        long long slot, tgt;
        if (!json_num(line, "slot", &slot) || !json_num(line, "target", &tgt)) {
            snprintf(out, cap, "{\"ok\":false,\"error\":\"slot/target\"}"); return; }
        tc_hijack_exit((int)slot, (uint32_t)tgt);
        uint32_t key = 0, rr = cur_room_record(&key), cur = 0;
        if (rr) rd32((void *)(uintptr_t)exit_target_addr(rr, (int)slot), &cur);
        snprintf(out, cap, "{\"ok\":%s,\"slot\":%d,\"target_room\":%u,\"room_key\":%u}",
                 rr ? "true" : "false", (int)slot, (unsigned)cur, key);
        return;
    }
    if (!strcmp(cmd, "revert")) {
        // {"cmd":"revert","slot":N} — restore exit-slot N's original target_room.
        long long slot;
        if (!json_num(line, "slot", &slot)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no slot\"}"); return; }
        tc_revert_exit((int)slot);
        uint32_t key = 0, rr = cur_room_record(&key), cur = 0;
        if (rr) rd32((void *)(uintptr_t)exit_target_addr(rr, (int)slot), &cur);
        snprintf(out, cap, "{\"ok\":%s,\"slot\":%d,\"target_room\":%u}", rr ? "true" : "false", (int)slot, (unsigned)cur);
        return;
    }
    if (!strcmp(cmd, "rooms")) {
        // {"cmd":"rooms"[,"max":M]} — enumerate the room-record table (valid warp destinations):
        // per room {key, area, scene}.  Walks the contiguous 0x150-B table seeded from the current room.
        long long m = 512; json_num(line, "max", &m);
        if (m < 1) m = 1;
        if (m > 1024) m = 1024;
        static tc_room rooms[1024];
        int nr = tc_get_rooms(rooms, (int)m);
        int n = snprintf(out, cap, "{\"ok\":true,\"count\":%d,\"rooms\":[", nr);
        for (int i = 0; i < nr && n < cap - 48; ++i)
            n += snprintf(out + n, cap - n, "%s{\"key\":%u,\"area\":%u,\"scene\":%u}",
                          i ? "," : "", rooms[i].key, rooms[i].area, rooms[i].scene);
        snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "view")) {
        // {"cmd":"view"[,"off":0x104c]} — mouse-fly camera diagnostic.  Reports the resolved world-view
        // rect (left/top from cur_x/cur_y + span) from the camera object *(render_root+off), plus the
        // player box for a sanity check (box_x should sit inside [left, left+vw]).  `off` tunes the
        // render_root->camera pointer offset, live.
        long long off;
        if (json_num(line, "off", &off)) g_cam_off = (uint32_t)off;
        uint32_t root = 0, cam = 0; rd32(AP(VA_RENDER_ROOT), &root);
        if (root) rd32((void *)(uintptr_t)(root + g_cam_off), &cam);
        uintptr_t base = find_player(); uint32_t box = 0, bx = 0, btop = 0;
        if (base && rd32((void *)(base + OFF_BOX), &box) && box > 0x10000) {
            rd32((void *)((uintptr_t)box + BOX_X), &bx);
            rd32((void *)((uintptr_t)box + BOX_TOP), &btop);
        }
        int left = 0, top = 0, vw = 0, vh = 0, cok = read_view(&left, &top, &vw, &vh);
        int n = snprintf(out, cap, "{\"ok\":true,\"render_root\":\"0x%08x\",\"cam_obj\":\"0x%08x\","
                         "\"cam_off\":\"0x%x\",\"cam_ok\":%s,"
                         "\"view_left\":%d,\"view_top\":%d,\"view_w\":%d,\"view_h\":%d,"
                         "\"box_x\":%d,\"box_top\":%d,\"cam_fields\":[",
                         root, cam, g_cam_off, cok ? "true" : "false",
                         left, top, vw, vh, (int)bx, (int)btop);
        for (uint32_t o = 0x10; cam && o <= 0x70 && n < cap - 64; o += 4) {   // dump the camera object
            uint32_t v = 0; rd32((void *)(uintptr_t)(cam + o), &v);
            n += snprintf(out + n, cap - n, "%s{\"off\":\"0x%x\",\"val\":%d}", o == 0x10 ? "" : ",", o, (int)v);
        }
        snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "unlock_all")) { lock_clear_all(); g_god = 0; snprintf(out, cap, "{\"ok\":true}"); return; }
    if (!strcmp(cmd, "call")) {
        // {"cmd":"call","va":"0x585cf0","a0":..,"a1":..,...,"ecx":"0x..","reloc":true}
        // Calls the engine fn; args a0..a7 (contiguous), ecx=this (thiscall).  Now runs on the
        // ENGINE thread (enqueued + drained at the inputPoll safepoint), so calling non-reentrant
        // engine code is safe — no longer the socket-thread hazard.
        long long va, tmp;
        if (!json_num(line, "va", &va)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no va\"}"); return; }
        int reloc = json_bool(line, "reloc", 1);        // module VA -> AP()-relocate by default
        uintptr_t fn = reloc ? (uintptr_t)AP(va) : (uintptr_t)va;
        if (!mem_readable((void *)fn, 1)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"fn unreadable\"}"); return; }
        eq_job *j = eq_alloc(EQ_CALL);
        if (!j) { snprintf(out, cap, "{\"ok\":false,\"error\":\"queue full\"}"); return; }
        int n = 0; char key[4];
        for (int i = 0; i < 8; ++i) {                   // a0..a7 -> u[3..10]
            snprintf(key, sizeof key, "a%d", i);
            if (!json_num(line, key, &tmp)) break;
            j->u[3 + n++] = (uint32_t)tmp;
        }
        j->u[0] = (uint32_t)fn;
        j->u[1] = json_num(line, "ecx", &tmp) ? (uint32_t)tmp : 0;
        j->u[2] = (uint32_t)n;
        uint32_t r = engine_wait(j, 2000);              // engine calls can be slow; generous timeout
        snprintf(out, cap, "{\"ok\":true,\"ret\":%u,\"ret_hex\":\"0x%08x\",\"fn\":\"0x%08x\",\"argc\":%d}",
                 r, r, (unsigned)fn, n);
        return;
    }

    if (!strcmp(cmd, "saves")) {
        // {"cmd":"saves"} — enumerate + identify EVERY on-disk save (no engine load).
        // Reads user\savedataNN.sdt directly (the standalone sotes_save decoder), so the
        // agent/UI can see what each slot is and pick one to `load`.
        int n = snprintf(out, cap, "{\"ok\":true,\"saves\":[");
        int first = 1;
        for (int slot = 0; slot < 16 && n < cap; ++slot) {
            char path[512]; trainer_save_path(slot, path, sizeof path);
            FILE *f = fopen(path, "rb"); if (!f) continue; fclose(f);
            char sj[640]; save_json_one(slot, sj, sizeof sj);
            n += snprintf(out + n, cap - n, "%s%s", first ? "" : ",", sj); first = 0;
        }
        if (n < cap) snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "saveinfo")) {
        // {"cmd":"saveinfo","slot":N} — full summary of one slot.
        long long slot;
        if (!json_num(line, "slot", &slot)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no slot\"}"); return; }
        char sj[640]; save_json_one((int)slot, sj, sizeof sj);
        snprintf(out, cap, "{\"ok\":true,\"save\":%s}", sj);
        return;
    }

    if (!strcmp(cmd, "load")) {
        // {"cmd":"load"[,"slot":N][,"downs":N]} — MENU-DRIVE (the game's own load path, no
        // risky direct engine calls): inject confirm at the TITLE (0x437c70) -> Continue ->
        // the save-slot picker; then at the PICKER (0x4378d0) load the target save.
        //   "slot":N  -> select savedataNN via the RE'd picker selection model, then confirm
        //               (any slot; VERIFIED default path, slot-targeting RE'd — see DESIGN).
        //   no slot   -> load the default-highlighted (newest) save (the VERIFIED tower path).
        //   "downs":N -> manual: rotate the picker N times before confirm (fallback nav).
        long long tmp;
        int slot  = json_num(line, "slot",  &tmp) ? (int)tmp : -1;
        int downs = json_num(line, "downs", &tmp) ? (int)tmp : 0;
        int loaded = menu_load(slot, downs, NULL, 0);    // canonical drive (shared with the UI)
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":%s,\"loaded\":%s,\"picker\":\"%s\",\"player\":%s}",
                 loaded ? "true" : "false", loaded ? "true" : "false", g_pk_diag, pj);
        return;
    }
    if (!strcmp(cmd, "newgame")) {
        // {"cmd":"newgame"[,"to":N]} — from the TITLE, rotate N times to the "New Game"/Start
        // item then confirm (the game's own menu).  "to" = title rotations from the default
        // (Continue) to Start (default 1).  With dlgskip on, the intro cutscene skips itself.
        long long tmp;
        int to  = json_num(line, "to",  &tmp) ? (int)tmp : 1;    // rotations to New Game
        int btn = json_num(line, "btn", &tmp) ? (int)tmp : 0x04; // 2=up / 4=down
        int gap = json_num(line, "gap", &tmp) ? (int)tmp : 6;    // polls between rotates
        int started = menu_newgame(to, btn, gap);               // canonical drive (shared with the UI)
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":%s,\"started\":%s,\"player\":%s}",
                 started ? "true" : "false", started ? "true" : "false", pj);
        return;
    }
    if (!strcmp(cmd, "loadraw")) {
        // {"cmd":"loadraw","slot":N[,"enter":true]} — EXPERIMENTAL direct chain: alloc S ->
        // 416550 load -> 586c60 apply -> (enter) 0x5cb460 transition, on the safepoint.
        // load+apply are verified; the standalone transition CRASHES (needs the picker
        // dispatcher `this` + slot-index arg) — prefer `load`.  See DESIGN.md session 3b.
        long long slot;
        if (!json_num(line, "slot", &slot)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no slot\"}"); return; }
        ensure_hooks();
        g_load_log[0] = 0; g_load_ret = 0;
        g_load_slot = (int)slot;
        g_load_enter = json_bool(line, "enter", 0);   // default: load+apply only (crash-safe)
        g_load_pending = 1;
        for (int i = 0; i < 300 && g_load_pending; ++i) Sleep(10);   // await the safepoint (<100ms at title)
        int ran = !g_load_pending;
        if (!ran) { g_load_pending = 0; snprintf(g_load_log, sizeof g_load_log, "safepoint never fired (game not presenting / not at a scene?)"); }
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap,
            "{\"ok\":%s,\"ran\":%s,\"ret\":%u,\"dispatch_this\":\"0x%08x\",\"log\":\"%s\",\"player\":%s}",
            ran ? "true" : "false", ran ? "true" : "false", g_load_ret,
            (unsigned)g_dispatch_this, g_load_log, pj);
        return;
    }
    if (!strcmp(cmd, "keepactive")) {
        // {"cmd":"keepactive","on":true|false} — re-post WM_ACTIVATEAPP(TRUE) so the game
        // keeps rendering/updating while unfocused, without stealing focus.  Default ON.
        g_keepactive = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"keepactive\":%s}", g_keepactive ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "dlgtrace")) {
        // {"cmd":"dlgtrace"} — dump the text-grid trace: for each grid+off column that CHANGED over
        // the captured frames, report off + first/last/min/max.  The typewriter REVEAL counter = a
        // column that climbs first->last (min=first, max=last); its total is a sibling (const == the
        // char count).  Capture: dlgskip OFF, trigger a dialogue, let it type, advance with Enter.
        int n = g_dlgt_n; if (n > DLGT_N) n = DLGT_N;
        int used = snprintf(out, cap, "{\"ok\":true,\"grid\":\"0x%08x\",\"frames\":%d,\"changed\":[",
                            (unsigned)g_dlg_grid, n), first = 1;
        for (int c = 0; c < DLGT_W && used < (int)cap - 96; c++) {
            uint32_t f = g_dlgt[0][c], l = n ? g_dlgt[n - 1][c] : f, mn = f, mx = f;
            int chg = 0;
            for (int r = 0; r < n; r++) {
                uint32_t v = g_dlgt[r][c];
                if (v != f) chg = 1;
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            if (!chg) continue;
            used += snprintf(out + used, cap - used,
                "%s{\"off\":\"0x%x\",\"first\":%u,\"last\":%u,\"min\":%u,\"max\":%u}",
                first ? "" : ",", c * 4, f, l, mn, mx);
            first = 0;
        }
        snprintf(out + used, cap - used, "]}");
        return;
    }
    if (!strcmp(cmd, "dlgskip")) {
        // {"cmd":"dlgskip","on":true|false} — auto-skip dialogue: inject the dialogue advance
        // buttons (0x24/0x27) every gameplay poll, so any dialogue advances the instant it opens.
        // Default ON.  Harmless outside dialogue (those ids are dialogue-only).
        g_dlgskip = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"dlgskip\":%s}", g_dlgskip ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "dlgbtns")) {
        // {"cmd":"dlgbtns","b0":..,"b1":..,...} — set which button ids dlgskip injects each poll
        // (0/absent = clear that slot).  For tuning the reveal-skip button set live.
        long long tmp; char key[4];
        for (int i = 0; i < 6; ++i) {
            snprintf(key, sizeof key, "b%d", i);
            g_dlg_btns[i] = json_num(line, key, &tmp) ? (int)tmp : 0;
        }
        int n = snprintf(out, cap, "{\"ok\":true,\"dlgbtns\":[");
        for (int i = 0; i < 6; ++i) if (g_dlg_btns[i]) n += snprintf(out+n, cap-n, "%s%d", n && out[n-1]!='[' ? "," : "", g_dlg_btns[i]);
        snprintf(out+n, cap-n, "]}");
        return;
    }
    if (!strcmp(cmd, "press")) {
        // {"cmd":"press","btn":N[,"n":count]} — inject button N into the active input mgr `count`
        // times over the next polls (probe: find which button does what in the current context).
        long long btn, cnt = 1;
        if (!json_num(line, "btn", &btn)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no btn\"}"); return; }
        json_num(line, "n", &cnt);
        uint32_t mgr = g_ti_mgr;
        for (int i = 0; i < (int)cnt && mgr; ++i) {
            uint32_t nowv = 0, e = g_ti_esp; if (e) rd32((void *)(uintptr_t)(e + 4), &nowv);
            inject_record(mgr, (uint32_t)btn, nowv, 8);
            Sleep(16);
        }
        snprintf(out, cap, "{\"ok\":true,\"pressed\":%d,\"n\":%d,\"mgr\":\"0x%08x\"}", (int)btn, (int)cnt, (unsigned)mgr);
        return;
    }
    if (!strcmp(cmd, "attract")) {
        // {"cmd":"attract","freeze":true|false} — patch/unpatch the title idle->demo
        // trigger so the title stays up indefinitely (no engine hooks needed).
        int freeze = json_bool(line, "freeze", 1);
        attract_freeze(freeze);
        snprintf(out, cap, "{\"ok\":true,\"attract_frozen\":%s}", freeze ? "true" : "false");
        return;
    }

    // ── scanner + fast-skip (hook-free; the ctor hook will move to MinHook) ──
    if (!strcmp(cmd, "scan")) {
        // {"cmd":"scan","value":N[,"max":M]} — find every dword == value in the game heap.
        long long v, m = 32;
        if (!json_num(line, "value", &v)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no value\"}"); return; }
        json_num(line, "max", &m);
        if (m < 1) m = 1;
        if (m > 256) m = 256;
        uintptr_t hits[256];
        int nh = scan_u32((uint32_t)v, hits, (int)m);
        int n = snprintf(out, cap, "{\"ok\":true,\"value\":\"0x%08x\",\"count\":%d,\"addrs\":[",
                         (unsigned)(uint32_t)v, nh);
        for (int i = 0; i < nh && n < cap - 16; ++i)
            n += snprintf(out + n, cap - n, "%s\"0x%08x\"", i ? "," : "", (unsigned)hits[i]);
        snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "dlggrid") || !strcmp(cmd, "dlggrab")) {
        // {"cmd":"dlggrid"} — locate the live story/cutscene dialogue text-grid(s) by the structural
        // + body-color signature (works on an ALREADY-open box, no hook).  Sets g_dlg_grid to the
        // first hit for grid/fastskip.  (dlggrab is an alias — the old exec-BP capture is retired.)
        uintptr_t g[16];
        int ng = find_dlg_grids(g, 16);
        if (ng > 0) g_dlg_grid = (uint32_t)g[0];
        int n = snprintf(out, cap, "{\"ok\":true,\"count\":%d,\"grids\":[", ng);
        for (int i = 0; i < ng && n < cap - 128; ++i) {
            uint32_t cc = 0, total = 0, reveal = 0; int widx = -1;
            uint32_t tm = dlg_active_tm((uint32_t)g[i], &total, &reveal, &widx);
            rd32((void *)(g[i] + GRID_POOL_CC), &cc);
            n += snprintf(out + n, cap - n,
                "%s{\"grid\":\"0x%08x\",\"count\":%u,\"line\":%d,\"tm\":\"0x%08x\",\"total\":%u,\"reveal\":%u}",
                i ? "," : "", (unsigned)g[i], cc >> 16, widx, (unsigned)tm, total, reveal);
        }
        snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "grid")) {
        // {"cmd":"grid"} — inspect the captured container (g_dlg_grid): walk to the active typing
        // line's text-machine and report its total + reveal (the REAL typewriter reveal state).
        uint32_t g = g_dlg_grid, cc = 0, arr = 0, total = 0, reveal = 0; int widx = -1;
        uint32_t tm = dlg_active_tm(g, &total, &reveal, &widx);
        if (g) { rd32((void *)(uintptr_t)(g + GRID_POOL_CC), &cc);
                 rd32((void *)(uintptr_t)(g + GRID_WIDGET_ARR), &arr); }
        snprintf(out, cap,
            "{\"ok\":true,\"grid\":\"0x%08x\",\"cap\":%u,\"count\":%u,\"widgets\":\"0x%08x\","
            "\"line\":%d,\"tm\":\"0x%08x\",\"total\":%u,\"reveal\":%u}",
            (unsigned)g, cc & 0xffff, cc >> 16, (unsigned)arr, widx, (unsigned)tm, total, reveal);
        return;
    }
    if (!strcmp(cmd, "fastskip")) {
        // {"cmd":"fastskip","on":true|false} — instant text: each tick force the active dialogue
        // line's reveal to total (walk g_dlg_grid -> widget -> TM+0x14=TM+0x10).  Pure UI-state
        // write (no input ⇒ door-safe).  Pair with dlgskip to fully fast-forward.  Default OFF.
        g_fastskip = json_bool(line, "on", 1);
        uint32_t total = 0, reveal = 0;
        uint32_t tm = dlg_active_tm(g_dlg_grid, &total, &reveal, NULL);
        snprintf(out, cap, "{\"ok\":true,\"fastskip\":%s,\"grid\":\"0x%08x\",\"tm\":\"0x%08x\",\"total\":%u,\"reveal\":%u}",
                 g_fastskip ? "true" : "false", (unsigned)g_dlg_grid, (unsigned)tm, total, reveal);
        return;
    }
    if (!strcmp(cmd, "autoskip")) {
        // {"cmd":"autoskip","on":true|false} — auto-advance ALL story/cutscene/NPC dialogue hands-free
        // (the "hold TAB" skip): NOPs the advance-gate je@0x437740 so the story stepper advances every
        // frame.  The STRONGEST skip — no keypress, no per-line poking, skips whole conversations.
        // Pure code patch, no world input (verified: no stray triggers).  **Default ON** (USER); it also
        // auto-advances CHOICE boxes, so `autoskip off` when you need to read or pick one.
        dlg_autoskip(json_bool(line, "on", 1));
        snprintf(out, cap, "{\"ok\":true,\"autoskip\":%s}", g_autoskip ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "dlghook")) {
        // {"cmd":"dlghook","on":true|false} — MinHook the grid ctor 0x5e59c0 to capture g_dlg_grid at
        // BUILD time (robust; supersedes the dlggrid scan — catches waiting/instant boxes too).
        // Trigger a dialogue after enabling; `captures` counts ctor hits.
        const char *r = dlg_hook_enable(json_bool(line, "on", 1));
        snprintf(out, cap, "{\"ok\":true,\"result\":\"%s\",\"on\":%s,\"captures\":%d,\"grid\":\"0x%08x\"}",
                 r, g_dlg_hook_on ? "true" : "false", g_dlg_ctor_captures, (unsigned)g_dlg_grid);
        return;
    }

    snprintf(out, cap, "{\"ok\":false,\"error\":\"unknown cmd\"}");
}

// ─── localhost line-JSON server ─────────────────────────────────────────────
static DWORD WINAPI server_thread(void *unused) {
    (void)unused;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { vlog("[srv] WSAStartup failed"); return 0; }
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == INVALID_SOCKET) { vlog("[srv] socket failed"); return 0; }
    BOOL yes = TRUE; setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes);
    struct sockaddr_in sa; memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; sa.sin_port = htons(TRAINER_PORT);
    // Bind all interfaces: the dev/LLM driver runs in WSL2 and reaches the Windows
    // host over the LAN (cutestation.soy), which loopback would block.  A SHIPPED
    // build should default to INADDR_LOOPBACK (UI is same-host) + make this a config.
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ls, (struct sockaddr *)&sa, sizeof sa) != 0) { vlog("[srv] bind %d failed", TRAINER_PORT); closesocket(ls); return 0; }
    if (listen(ls, 4) != 0) { vlog("[srv] listen failed"); closesocket(ls); return 0; }
    vlog("[srv] listening on 127.0.0.1:%d", TRAINER_PORT);

    for (;;) {
        SOCKET cs = accept(ls, NULL, NULL);
        if (cs == INVALID_SOCKET) { Sleep(50); continue; }
        vlog("[srv] client connected");
        char buf[4096]; int used = 0;
        for (;;) {
            int n = recv(cs, buf + used, (int)sizeof buf - 1 - used, 0);
            if (n <= 0) break;
            used += n; buf[used] = 0;
            char *nl;
            while ((nl = memchr(buf, '\n', used)) != NULL) {
                *nl = 0;
                char resp[8192];   // large enough for `saves` (all slots) + summaries
                handle_line(buf, resp, sizeof resp);
                int rl = (int)strlen(resp); resp[rl++] = '\n';
                send(cs, resp, rl, 0);
                int rest = used - (int)(nl + 1 - buf);
                memmove(buf, nl + 1, rest); used = rest; buf[used] = 0;
            }
            if (used >= (int)sizeof buf - 1) used = 0;   // overlong line: drop
        }
        closesocket(cs);
        vlog("[srv] client disconnected");
    }
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    (void)r;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
        GetModuleFileNameA(NULL, g_logpath, MAX_PATH);
        char *slash = strrchr(g_logpath, '\\');
        strcpy(slash ? slash + 1 : g_logpath, "oss_trainer.log");
        InitializeCriticalSection(&g_lock_cs);
        InitializeCriticalSection(&g_hj_cs);
        InitializeCriticalSection(&g_eq_cs);                     // engine-thread execution queue
        vlog("[trainer] attach: base=%p delta=%p port=%d",
             GetModuleHandleA(NULL), (void *)g_delta, TRAINER_PORT);
        // NB: pokes/calls + freezes/mouse-fly run engine-side in the inputPoll safepoint
        // (poll_title_cb), NOT on a worker thread — see the engine-thread execution queue.
        CreateThread(NULL, 0, server_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, keepalive_thread, NULL, 0, NULL);  // launcher + keep-active + attract-off + hooks
        trainer_ui_start();                                      // the Dear ImGui window (own thread)
    }
    return TRUE;
}
