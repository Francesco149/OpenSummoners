export const meta = {
  name: 'opensummoners-subsystem-survey',
  description: 'Read-only fan-out: map every engine-proper band of sotes.exe to subsystems + scout the forward port path, to seed ROADMAP.md',
  phases: [
    { title: 'Map', detail: 'one Explore agent per address band → subsystem map' },
    { title: 'Scout', detail: 'one Explore agent per forward-path cluster → port-readiness card' },
  ],
}

// ---- shared context handed to every agent -------------------------------
const REPO = '/opt/src/OpenSummoners'
const CTX = `
You are surveying the Ghidra decompile of the Steamless-unpacked \`sotes.exe\`
(Fortune Summoners, Lizsoft 2008 / Carpe Fulgur 2012). This is an educational
RE + game-preservation project building a faithful C drop-in.

READ-ONLY. Do not edit any file. Do not build. Just read and report.

Where things are (all under ${REPO}):
- docs/decompiled/by-address/<va>.c — one decompiled function per entry VA
  (e.g. 0056aea0.c). THIS is your primary source — read the bodies.
- docs/decompiled/functions.csv — index: name,entry,size,is_thunk,calling_conv.
  Use it to enumerate the VAs in your assigned range and their sizes.
- docs/findings/*.md — existing RE writeups (asset-loader, ddraw-init,
  audio-init, winmain-and-bootstrap, title-scene, palette-session,
  engine-quirks, launcher-dialog, cpp-recovery-workflow, the 0057ca40 rabbit
  hole). Cross-reference but DON'T re-derive what's already documented there.
- docs/port-ledger.md — which VAs are already ported (status tested/ported).

Engine-proper code is below 0x5bdab0; above that is the linked MSVC CRT — ignore it.
The engine is a C++ binary: most "subsystems" are clusters of methods on a few
classes (vtables, this-call ctors/dtors). \`FUN_<va>\` tokens inside a body are
call edges. Image base 0x00400000.

Findings are decompile-grade: strong leads, but say so — exact struct offsets /
strides must be byte-verified before a port depends on them. Be concrete: cite
representative driver VAs (entry points, not every leaf), name the subsystem in
gameplay terms a player would recognise where you can, and note anything weird
(load-bearing bugs, hand-rolled hashes, odd retry loops, off-by-ones) as a quirk.
`

const MAP_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  required: ['band', 'one_liner', 'subsystems', 'quirks'],
  properties: {
    band: { type: 'string', description: 'the VA range you surveyed, e.g. 0x410000-0x420000' },
    one_liner: { type: 'string', description: 'one sentence: what dominates this band' },
    subsystems: {
      type: 'array',
      items: {
        type: 'object',
        additionalProperties: false,
        required: ['name', 'representative_vas', 'notes'],
        properties: {
          name: { type: 'string', description: 'subsystem in gameplay/engine terms' },
          representative_vas: {
            type: 'array',
            items: {
              type: 'object', additionalProperties: false, required: ['va', 'role'],
              properties: { va: { type: 'string' }, role: { type: 'string' } },
            },
          },
          notes: { type: 'string', description: 'class/vtable structure, key globals, port difficulty signal' },
        },
      },
    },
    quirks: {
      type: 'array',
      description: 'interesting/charming/load-bearing engine oddities found',
      items: {
        type: 'object', additionalProperties: false, required: ['title', 'va', 'desc'],
        properties: { title: { type: 'string' }, va: { type: 'string' }, desc: { type: 'string' } },
      },
    },
  },
}

const SCOUT_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  required: ['cluster', 'entry_va', 'summary', 'functions', 'unported_deps', 'difficulty', 'blocks', 'quirks'],
  properties: {
    cluster: { type: 'string' },
    entry_va: { type: 'string' },
    summary: { type: 'string', description: '2-4 sentences: what this cluster does and how the forward path reaches it' },
    functions: {
      type: 'array',
      description: 'the functions in this cluster worth porting, with role',
      items: {
        type: 'object', additionalProperties: false, required: ['va', 'size_hint', 'role'],
        properties: { va: { type: 'string' }, size_hint: { type: 'string' }, role: { type: 'string' } },
      },
    },
    unported_deps: { type: 'array', items: { type: 'string' }, description: 'VAs this cluster calls that are not yet ported' },
    difficulty: { type: 'string', enum: ['trivial', 'easy', 'moderate', 'hard', 'very-hard'] },
    blocks: { type: 'string', description: 'which milestone / visible result this unblocks' },
    quirks: {
      type: 'array',
      items: {
        type: 'object', additionalProperties: false, required: ['title', 'va', 'desc'],
        properties: { title: { type: 'string' }, va: { type: 'string' }, desc: { type: 'string' } },
      },
    },
  },
}

// ---- phase 1: map the engine-proper address bands ------------------------
phase('Map')

const BANDS = [
  ['0x401000', '0x410000', 'low-level class ctors/dtors, the earliest helpers, and per-screen UI composition'],
  ['0x410000', '0x420000', 'menu/UI widgets, dialog/string formatting, the FUN_00412c10 menu-controller family'],
  ['0x420000', '0x430000', 'game-state / scene objects / screen setup'],
  ['0x430000', '0x440000', 'frame & game logic, save/load, the FUN_0043c110 input poll family'],
  ['0x440000', '0x450000', 'menu/UI framework, entity/sprite, more UI'],
  ['0x450000', '0x470000', 'render/dialog/sprite helpers (two sparse bands merged)'],
  ['0x470000', '0x480000', 'UI / text / dialog render'],
  ['0x480000', '0x490000', 'sprite/animation, UI layer render'],
  ['0x490000', '0x4a0000', 'UI render dispatch + sprite/blit (dense band)'],
  ['0x4a0000', '0x4d0000', 'screen dispatch, asset/archive load, sim setup (sparse, merged)'],
  ['0x4d0000', '0x540000', 'gameplay/sim tail — very sparse, characterise what little is here'],
  ['0x540000', '0x560000', 'audio (DSound) and misc subsystems'],
  ['0x560000', '0x570000', 'title scene runner FUN_0056aea0 + wave-load + map/scene'],
  ['0x570000', '0x590000', '2D animation / sprite system, map/tile (two bands merged)'],
  ['0x590000', '0x5a0000', 'map/sprite render system (dense band)'],
  ['0x5a0000', '0x5bdab0', 'launcher settings parser, LUT formulas, DInput pad, ZDD renderer + app pump + bitmap/asset (much already ported)'],
]

const maps = await parallel(BANDS.map(([lo, hi, hint]) => () =>
  agent(
    `${CTX}\nMAP the engine-proper address band ${lo}-${hi}. Hint on what's likely here: ${hint}.\n` +
    `Enumerate the functions in this VA range from functions.csv, then read a representative sample of the ` +
    `by-address/<va>.c bodies (prioritise the largest functions and obvious driver/entry points — you do NOT ` +
    `need to read every leaf). Cluster them into subsystems. For each subsystem give 2-6 representative driver ` +
    `VAs with a one-line role, and notes on its class/vtable shape, key globals (DAT_/PTR_), and a port-difficulty ` +
    `signal. Skip thunks and tiny <=6-byte stubs. Report quirks you stumble on.`,
    { label: `map ${lo}`, phase: 'Map', agentType: 'Explore', schema: MAP_SCHEMA }
  )
)).then(rs => rs.filter(Boolean))

log(`Map phase done: ${maps.length}/${BANDS.length} bands surveyed`)

// ---- phase 2: scout the forward port path (title -> menu -> gameplay) ----
phase('Scout')

const CLUSTERS = [
  ['title scene runner', 'FUN_0056aea0',
    'The 3441-byte title-menu scene runner is the next big port (first visible frame). Read 0056aea0.c fully. ' +
    'Map its outer loop structure, its state variables, the PTR_DAT_0056bfa4 jumptable (7 inlined handlers — identify each), ' +
    'and the helpers FUN_0056c070 (sparkle) and FUN_0056c180 (title-menu flip). Cross-ref docs/findings/title-scene.md ' +
    'but go deeper. Which of its callees are still unported?'],
  ['menu controller', 'FUN_00412c10',
    'FUN_00412c10 is the menu-controller allocator the title runner calls. Read it and the surrounding 0x412***/0x414*** ' +
    'menu/UI family. What object does it build, what vtable, what feeds it? Identify the menu-item model and input wiring.'],
  ['input system', 'FUN_0043c110',
    'FUN_0043c110 (input poll) and FUN_0043ce50 (input action latch) plus the DInput pad lazy-attach FUN_005ba120 / ' +
    'FUN_005ba290. Read all four. Map the keyboard + gamepad input model: what state struct, what globals, how a ' +
    '"confirm" press is detected. This is needed for any interactive screen.'],
  ['wave-load / sprite pool', 'FUN_00563ef0',
    'FUN_00563ef0 is the wave-load; its second half is unported per HANDOFF. Read it and the sprite/asset pool it feeds ' +
    '(DAT_008a760c sprite asset pool, DAT_008a92b4 object pool). Cross-ref docs/findings/asset-loader.md. What asset ' +
    'format does it parse, what gets cached where?'],
  ['launcher settings parse', 'FUN_005a4770',
    'FUN_005a4770 (~45KB) is the launcher-settings/config.dat parser that the --launcher-mode CLI flag currently stubs. ' +
    'Read enough to map the record layout and which settings it reads. Cross-ref docs/formats/config-dat.md and ' +
    'docs/findings/launcher-dialog.md. How big a port is the real thing?'],
  ['audio bring-up', 'audio-init',
    'Scout the DirectSound audio subsystem entry and how boot reaches it. Cross-ref docs/findings/audio-init.md, then ' +
    'find the per-sound-effect and BGM playback paths in the 0x540000-0x560000 band. What is the smallest first audio ' +
    'milestone (e.g. title BGM), and what does it depend on?'],
]

const scouts = await parallel(CLUSTERS.map(([name, entry, brief]) => () =>
  agent(
    `${CTX}\nSCOUT the "${name}" cluster (entry ${entry}).\n${brief}\n` +
    `Produce a port-readiness card: a 2-4 sentence summary, the functions worth porting (VA + size hint + role), ` +
    `the still-unported VAs it depends on, an overall difficulty rating, and which milestone/visible result it unblocks. ` +
    `Report quirks.`,
    { label: `scout ${name}`, phase: 'Scout', agentType: 'Explore', schema: SCOUT_SCHEMA }
  )
)).then(rs => rs.filter(Boolean))

log(`Scout phase done: ${scouts.length}/${CLUSTERS.length} clusters scouted`)

return { maps, scouts }
