'use strict';

// EN-SE trainer agent — LLM-operable game manipulation for the retail
// Special Edition (sotes_en.exe, Steam buildid 23890965, ImageBase 0x400000).
//
// Attach-mode side-car: composes with tools/ennse_voice/repro_probe.py (which
// owns spawn/input/screenshots) or runs alone against any live instance.
// Engine addresses are validated by byte signature before any hook goes in.
//
// Capabilities: raw memory (read/write/block), value hunting (heap-wide scan +
// winnow), multi-slot freezing (god mode / coins / anything), block snapshots
// for offset discovery, a live actor registry (every 0x419b00 construction),
// spawn forcing (rewrite constructor arg 6 inside the monster band), position
// read/write once offsets are configured (teleport / bookmarks), and synthetic
// sound-set tests (construct + play through the real mixer).

const IMAGE_BASE = 0x400000;
const VA = {
    characterCreate: 0x419b00,   // 14-arg ctor; esp+0x18 = entity code (arg 6)
    soundSetInit: 0x423850,      // thiscall (key, x, y) -> owner+0x15b78
    soundPlay: 0x4075a0,         // thiscall (action, a2..a6)
    bufferStart: 0x5e3e10,       // SetCurrentPosition(0) + Play wrapper
};

// Durable anchors off the constructor-hooked player actor (no per-run hunting).
// The player is the actor constructed with Arche's protagonist code 0xc35a; from
// that base every stat/position is a fixed offset (verified live 2026-07-17,
// EN-SE buildid 23890965). Stat block matches the port's party_stats (party.h).
const PLAYER_CODE = 0xc35a;
const OFF = {
    world_x: 0xc76c,   // actor world x, centi-px (px*100); moves with L/R input
    world_y: 0xc770,   // actor world y, centi-px (candidate; changes with L/R)
    stat_block: 0x760, // actor -> party_stats
    hp_cur: 0x54,      // stat_block + ; current HP
    hp_base: 0x58,     // max HP term (party.h: +0x58/+0x84/+0x9c summed)
    mp_cur: 0x5c,
    level: 0xe0,
};
const SIG = {
    characterCreate: [0x83, 0xec, 0x20, 0x53, 0x55, 0x8b, 0xe9, 0x33, 0xdb, 0x56, 0x57],
    soundSetInit: [0x83, 0xec, 0x08, 0x53, 0x55, 0x56, 0x57, 0x8b, 0xf9],
    soundPlay: [0x81, 0xec, 0x98, 0x00, 0x00, 0x00, 0x8b, 0x84, 0x24],
    bufferStart: [0x56, 0x8b, 0xf1, 0x6a, 0x00, 0x8b, 0x06],
};

let base = null;
function ap(va) { return base.add(va - IMAGE_BASE); }
function emit(kind, fields) { send(Object.assign({kind: kind}, fields || {})); }

function validate() {
    // A composing agent (repro_probe) may already have trampolined some of
    // these entries (first byte 0xe9 = a relative jmp Frida wrote).  That is
    // expected in side-car mode — only flag an entry whose UNHOOKED bytes
    // diverge from the known build.
    const hooked = [];
    for (const name of Object.keys(SIG)) {
        const got = new Uint8Array(ap(VA[name]).readByteArray(SIG[name].length));
        if (got[0] === 0xe9) { hooked.push(name); continue; }
        for (let i = 0; i < SIG[name].length; ++i) {
            if (got[i] !== SIG[name][i])
                throw new Error('signature mismatch @' + name +
                                ' byte ' + i + ': got 0x' + got[i].toString(16));
        }
    }
    return hooked;
}

// ── actor registry ────────────────────────────────────────────────────────
// Every character construction is recorded (party, NPCs, monsters alike).
// The monster band for source=0 spawn forcing (exact codes still accepted).
function isMonsterCode(code) {
    return (code >= 0xc742 && code <= 0xc83e) ||
           (code >= 0x18744 && code <= 0x1875d);
}
const actors = new Map();   // owner -> {code, requested, ms, forced}
let playerActor = null;     // the code-0xc35a actor base (durable anchor)
let startMs = Date.now();
let spawnForceQueue = [];   // {id, target, source, remaining}
let spawnForceSeq = 0;

function installActorHooks() {
    Interceptor.attach(ap(VA.characterCreate), {
        onEnter() {
            this.owner = this.context.ecx;
            this.originalCode = this.context.esp.add(0x18).readU32();
            this.force = null;
            if (spawnForceQueue.length) {
                const req = spawnForceQueue[0];
                const match = req.source === 0
                    ? isMonsterCode(this.originalCode)
                    : req.source === this.originalCode;
                if (match) {
                    this.context.esp.add(0x18).writeU32(req.target);
                    this.force = req.id;
                    req.remaining -= 1;
                    if (req.remaining <= 0) spawnForceQueue.shift();
                }
            }
        },
        onLeave() {
            let finalCode = null;
            try { finalCode = this.owner.add(0x1d4).readU32(); } catch (_) {}
            actors.set(this.owner.toString(), {
                code: finalCode,
                original: this.originalCode,
                forced: this.force,
                ms: Date.now() - startMs,
            });
            if (finalCode === PLAYER_CODE) {
                playerActor = this.owner;
                emit('player_found', {actor: playerActor.toString()});
            }
            if (this.force !== null || isMonsterCode(this.originalCode) ||
                (finalCode !== null && isMonsterCode(finalCode))) {
                emit('actor_created', {owner: this.owner.toString(),
                                       original: this.originalCode,
                                       code: finalCode, forced: this.force});
            }
        },
    });
    Interceptor.attach(ap(VA.soundSetInit), {
        onEnter() { this.owner = this.context.ecx; },
        onLeave() {
            const a = actors.get(this.owner.toString());
            if (a !== undefined) {
                try { a.soundKey = null; } catch (_) {}
            }
        },
    });
}

// ── freezes ───────────────────────────────────────────────────────────────
const freezes = new Map();  // name -> {addr, value, type}
setInterval(function () {
    for (const [name, f] of freezes) {
        try {
            if (f.type === 'u16') ptr(f.addr).writeU16(f.value & 0xffff);
            else ptr(f.addr).writeU32(f.value >>> 0);
        } catch (_) {
            freezes.delete(name);
            emit('freeze_dead', {name: name, addr: f.addr});
        }
    }
}, 50);

// ── snapshots (offset discovery) ──────────────────────────────────────────
const snaps = new Map();    // name -> {base, words: Uint32Array}

// ── synthetic sound tests ─────────────────────────────────────────────────
let soundSetInitFn = null, soundPlayFn = null;
const syntheticOwners = [];
let pendingSoundTests = [];
function installSoundSafepoint() {
    // soundPlay runs on the game thread constantly during play; bufferStart
    // does too.  Use characterCreate-independent safepoint: hook bufferStart
    // and inputless soundPlay entries are too rare in quiet scenes, so also
    // service from a 100ms interval via the main thread is NOT safe — engine
    // calls must run on the game thread.  We service pending tests on entry
    // to soundPlay OR bufferStart, whichever comes first.
    function service() {
        const t = pendingSoundTests.shift();
        if (t === undefined) return;
        try {
            const owner = Memory.alloc(0x15b90);
            syntheticOwners.push(owner);
            soundSetInitFn(owner, t.key, 0, 0);
            const so = owner.add(0x15b78).readPointer();
            if (so.isNull()) throw new Error('null sound object');
            const result = soundPlayFn(so, t.action, 1, 0, 0, 0, 0);
            emit('sound_test_done', {id: t.id, key: t.key, action: t.action,
                                     result: result});
        } catch (e) {
            emit('sound_test_done', {id: t.id, key: t.key, action: t.action,
                                     error: '' + e});
        }
    }
    Interceptor.attach(ap(VA.soundPlay), {onEnter() { service(); }});
    Interceptor.attach(ap(VA.bufferStart), {onEnter() { service(); }});
}
let soundTestSeq = 0;

rpc.exports = {
    init() {
        base = Process.mainModule.base;
        const hooked = validate();
        soundSetInitFn = new NativeFunction(ap(VA.soundSetInit), 'void',
            ['pointer', 'uint32', 'int32', 'int32'], 'thiscall');
        soundPlayFn = new NativeFunction(ap(VA.soundPlay), 'int',
            ['pointer', 'uint32', 'int32', 'int32', 'int32', 'int32', 'int32'],
            'thiscall');
        installActorHooks();
        installSoundSafepoint();
        emit('trainer_ready', {base: base.toString(), preHooked: hooked});
        return true;
    },

    // raw memory
    read(addr, type) {
        const p = ptr(addr);
        switch (type || 'u32') {
            case 'u8': return p.readU8();
            case 'u16': return p.readU16();
            case 'i32': return p.readS32();
            case 'f32': return p.readFloat();
            case 'ptr': return p.readPointer().toString();
            default: return p.readU32();
        }
    },
    write(addr, value, type) {
        const p = ptr(addr);
        switch (type || 'u32') {
            case 'u8': p.writeU8(value & 0xff); break;
            case 'u16': p.writeU16(value & 0xffff); break;
            case 'f32': p.writeFloat(value); break;
            default: p.writeU32(value >>> 0); break;
        }
        return true;
    },
    readblock(addr, size) {
        return Array.from(new Uint32Array(ptr(addr).readByteArray(size)));
    },

    // value hunting (heap-wide)
    hunt(value) {
        const pat = [value & 0xff, (value >> 8) & 0xff,
                     (value >> 16) & 0xff, (value >> 24) & 0xff]
            .map(b => ('0' + b.toString(16)).slice(-2)).join(' ');
        const out = [];
        for (const r of Process.enumerateRanges('rw-')) {
            if (r.size > 0x4000000) continue;
            let ms;
            try { ms = Memory.scanSync(r.base, r.size, pat); } catch (_) { continue; }
            for (const m of ms) {
                if (m.address.and(3).toInt32() === 0) out.push(m.address.toString());
                if (out.length >= 20000) return out;
            }
        }
        return out;
    },
    winnow(addrs, value) {
        const out = [];
        for (const a of addrs) {
            try { if (ptr(a).readU32() === (value >>> 0)) out.push(a); } catch (_) {}
        }
        return out;
    },

    // freezes
    freezeset(name, addr, value, type) {
        freezes.set(name, {addr: addr, value: value, type: type || 'u32'});
        return true;
    },
    freezedel(name) { return freezes.delete(name); },
    freezelist() {
        return Array.from(freezes.entries()).map(e => ({
            name: e[0], addr: e[1].addr, value: e[1].value, type: e[1].type}));
    },

    // snapshots
    snap(name, addr, size) {
        snaps.set(name, {base: addr,
                         words: Array.from(new Uint32Array(ptr(addr).readByteArray(size)))});
        return true;
    },
    snapdiff(name, mode) {
        const s = snaps.get(name);
        if (s === undefined) throw new Error('no snapshot ' + name);
        const now = new Uint32Array(ptr(s.base).readByteArray(s.words.length * 4));
        const out = [];
        for (let i = 0; i < now.length; ++i) {
            const a = s.words[i], b = now[i];
            if (a === b) continue;
            if (mode === 'dec' && b >= a) continue;
            if (mode === 'inc' && b <= a) continue;
            out.push({off: i * 4, was: a, now: b});
            if (out.length >= 400) break;
        }
        return out;
    },

    // actors / spawning
    actors(monstersOnly) {
        const out = [];
        for (const [owner, a] of actors) {
            if (monstersOnly && !(isMonsterCode(a.code >>> 0) ||
                                  isMonsterCode(a.original >>> 0))) continue;
            out.push(Object.assign({owner: owner}, a));
        }
        return out;
    },
    actorclear() { actors.clear(); return true; },
    forcespawn(target, source, count) {
        if (!isMonsterCode(target >>> 0))
            throw new Error('target is not an EN-SE monster code');
        spawnForceSeq += 1;
        spawnForceQueue.push({id: spawnForceSeq, target: target >>> 0,
                              source: source >>> 0,
                              remaining: Math.max(1, Math.min(count | 0, 32))});
        return spawnForceSeq;
    },
    forceclear() { spawnForceQueue = []; return true; },

    // ── player anchor (durable; no hunting) ──
    // Resolves the code-0xc35a actor captured at construction; falls back to a
    // hint pointer if the trainer attached after the player already existed.
    playerset(actorHint) {
        if (actorHint) playerActor = ptr(actorHint);
        return playerActor ? playerActor.toString() : null;
    },
    player() {
        if (playerActor === null) return null;
        const a = playerActor;
        let sb = null;
        try { sb = a.add(OFF.stat_block).readPointer(); } catch (_) {}
        const rd = (p, o) => { try { return p.add(o).readS32(); } catch (_) { return null; } };
        return {
            actor: a.toString(),
            world_x: rd(a, OFF.world_x), world_y: rd(a, OFF.world_y),
            stat_block: sb ? sb.toString() : null,
            hp: sb ? rd(sb, OFF.hp_cur) : null,
            hp_max: sb ? rd(sb, OFF.hp_base) : null,
            mp: sb ? rd(sb, OFF.mp_cur) : null,
            level: sb ? rd(sb, OFF.level) : null,
        };
    },
    // teleport: absolute centi-px, or nudge by dx/dy px.
    teleport(x, y, relative) {
        if (playerActor === null) throw new Error('player not found yet');
        const a = playerActor;
        if (relative) {
            if (x !== null && x !== undefined)
                a.add(OFF.world_x).writeS32(a.add(OFF.world_x).readS32() + x * 100);
            if (y !== null && y !== undefined)
                a.add(OFF.world_y).writeS32(a.add(OFF.world_y).readS32() + y * 100);
        } else {
            if (x !== null && x !== undefined) a.add(OFF.world_x).writeS32(x);
            if (y !== null && y !== undefined) a.add(OFF.world_y).writeS32(y);
        }
        return this.player();
    },
    // set a player stat (hp/hp_max/mp/level) and optionally lock it.
    setstat(which, value, lock) {
        if (playerActor === null) throw new Error('player not found yet');
        const sb = playerActor.add(OFF.stat_block).readPointer();
        const off = {hp: OFF.hp_cur, hp_max: OFF.hp_base, mp: OFF.mp_cur,
                     level: OFF.level}[which];
        if (off === undefined) throw new Error('unknown stat ' + which);
        sb.add(off).writeS32(value);
        if (lock) freezes.set('player_' + which,
                              {addr: sb.add(off).toString(), value: value, type: 'u32'});
        return this.player();
    },
    // teleport to the nearest OTHER live actor (nearest mob), by world_x.
    tpnearest(monstersOnly) {
        if (playerActor === null) throw new Error('player not found yet');
        const px = playerActor.add(OFF.world_x).readS32();
        let best = null, bestD = Infinity;
        for (const [owner, a] of actors) {
            if (owner === playerActor.toString()) continue;
            if (monstersOnly && !(isMonsterCode(a.code >>> 0))) continue;
            let x;
            try { x = ptr(owner).add(OFF.world_x).readS32(); } catch (_) { continue; }
            const d = Math.abs(x - px);
            if (d < bestD) { bestD = d; best = {owner: owner, x: x, code: a.code}; }
        }
        if (best === null) return null;
        // Land just beside the target, not on top of it.
        playerActor.add(OFF.world_x).writeS32(best.x - 3000);
        return {target: best, player: this.player()};
    },

    // synthetic sound test (serviced on the game thread at the next sound call)
    soundtest(key, action) {
        soundTestSeq += 1;
        pendingSoundTests.push({id: soundTestSeq, key: key >>> 0,
                                action: action >>> 0});
        return soundTestSeq;
    },
};
