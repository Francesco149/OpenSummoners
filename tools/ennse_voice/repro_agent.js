'use strict';

// EN-SE retail-only live probe for the Japanese-voice monster-SFX repro.
//
// Engine addresses below are only used after byte-signature validation against
// the verified unpacked Steam EN-SE executable.  Window hiding/activation comes
// from tools/frida/opensummoners-agent.js.  The DirectDraw capture path is found
// dynamically through DirectDrawCreateEx and COM vtables, so it does not borrow
// renderer addresses from the older retail build or from the open-source port.

const IMAGE_BASE = 0x400000;
const VA = {
    registrarEnter: 0x59cc8c,
    registrarBankCheck: 0x59ccaa,
    registrarExit: 0x59cd6f,
    bankLoad: 0x5d8b10,
    bankGlobal: 0x92af80,
    gateRoot: 0x92af98,
    table: 0x65b0e8,
    inputPoll: 0x437c70,
    keyDown: 0x5e2a70,
    mainGameInputBoundary: 0x43681d,
    mysteryDungeonInputBoundary: 0x5b66bd,
    genericMenuController: 0x4378d0,
    continueMenu: 0x585cf0,
    saveRead: 0x416550,
    saveValidate: 0x57f020,
    saveDispatch: 0x586c60,
    characterCreate: 0x419b00,
    soundSetInit: 0x423850,
    soundPlay: 0x4075a0,
    bufferStart: 0x5e3e10,
};

const SOUND_RECORD_SIZE = 0x24;
const RING_BASE_OFF = 0x0c;
const RING_SLOTS = 64;
const INPUT_RECORD_SIZE = 0x0c;
const INPUT_POOL_RECORDS = 256;

// IDirectDraw7 / IDirectDrawSurface7 vtable slots.  These are public DirectX 7
// ABI slots, not game-specific reverse-engineered addresses.
const V_DDRAW_CREATE_SURFACE = 6;
const V_SURF_ADDREF = 1;
const V_SURF_FLIP = 11;
const V_SURF_GETDC = 17;
const V_SURF_GETSURFACEDESC = 22;
const V_SURF_RELEASEDC = 26;
const DDSD2_SIZE = 124;
const DDSD2_HEIGHT_OFF = 8;
const DDSD2_WIDTH_OFF = 12;
const DDSD2_CAPS_OFF = 104;
const DIB_RGB_COLORS = 0;
const SRCCOPY = 0x00cc0020;

let base = null;
let startedMs = 0;
let registrarRun = 0;
let currentRegistrar = null;
let lastBank = '0x0';
let bankPoll = null;

let inputManager = null;
let inputPool = null;
let inputPoolIndex = 0;
let queuedPresses = [];
let polledPresses = [];
let queuedMenuPressBatches = [];
let queuedGameplayPressBatches = [];
let heldScancodes = new Set();
let inputPollCount = 0;
let activeInputManagerKey = null;
let activePollButtons = {};
let lastEngineNow = 0;

let soundSetInitFn = null;
let soundPlayFn = null;
let soundTestSequence = 0;
let soundTestQueue = [];
let syntheticOwners = [];
let soundSetsByObject = new Map();
let activeSoundCalls = new Map();
let hookedDirectSoundPlayFns = new Set();

let spawnForceSequence = 0;
let spawnForceQueue = [];
let forcedOwners = new Map();

let gdi = null;
let ddrawObjects = [];
let screenSurface = null;
let surfaceCreateCount = 0;
let hookedCreateSurfaceFns = new Set();
let hookedFlipFns = new Set();
let flipCount = 0;
let shotSequence = 0;
let shotQueue = [];
let servicingShot = false;

function ap(va) {
    return base.add(va - IMAGE_BASE);
}

function nowMs() {
    return Date.now() - startedMs;
}

function emit(kind, fields, data) {
    const out = Object.assign({
        kind: kind,
        ms: nowMs(),
        tid: Process.getCurrentThreadId(),
    }, fields || {});
    if (data === undefined) send(out);
    else send(out, data);
}

function readPtrSafe(p) {
    try { return p.readPointer(); } catch (_) { return ptr(0); }
}

function readUtf8Safe(p) {
    try { return p.isNull() ? null : p.readUtf8String(); } catch (_) { return null; }
}

function gateValue() {
    try {
        const p = ap(VA.gateRoot).readPointer();
        if (p.isNull()) return null;
        const q = p.readPointer();
        if (q.isNull()) return null;
        return q.add(0x238).readS32();
    } catch (_) {
        return null;
    }
}

function bankValue() {
    return readPtrSafe(ap(VA.bankGlobal));
}

function validateBytes(va, expected) {
    const got = new Uint8Array(ap(va).readByteArray(expected.length));
    for (let i = 0; i < expected.length; ++i) {
        if (got[i] !== expected[i]) {
            throw new Error('build signature mismatch @0x' + va.toString(16) +
                            ': byte ' + i + ' got 0x' + got[i].toString(16) +
                            ' expected 0x' + expected[i].toString(16));
        }
    }
}

function validateBuild() {
    validateBytes(VA.registrarEnter, [0xa1, 0xe8, 0xb0, 0x65, 0x00]);
    validateBytes(VA.registrarBankCheck, [0x39, 0x1d, 0x80, 0xaf, 0x92, 0x00]);
    validateBytes(VA.registrarExit, [0x5f, 0x5e, 0x5d, 0x5b]);
    validateBytes(VA.bankLoad, [0x81, 0xec, 0x10, 0x07, 0x00, 0x00]);
    validateBytes(VA.inputPoll, [0x53, 0x8b, 0x5c, 0x24, 0x0c, 0x55]);
    validateBytes(VA.keyDown,
                  [0x8b, 0x44, 0x24, 0x04, 0x25, 0xff, 0x00, 0x00, 0x00]);
    validateBytes(VA.mainGameInputBoundary,
                  [0x8b, 0x0d, 0xc8, 0xd5, 0x92, 0x00, 0x8b, 0xe8]);
    validateBytes(VA.mysteryDungeonInputBoundary,
                  [0x8b, 0xe8, 0xb9, 0x68, 0xac, 0x92, 0x00]);
    validateBytes(VA.genericMenuController,
                  [0x8b, 0x44, 0x24, 0x18, 0x53, 0x55, 0x56, 0x57]);
    validateBytes(VA.continueMenu, [0x6a, 0xff, 0x68, 0xac, 0xc4, 0x5f, 0x00]);
    validateBytes(VA.saveRead, [0x6a, 0xff, 0x68, 0x3b, 0xc1, 0x5f, 0x00]);
    validateBytes(VA.saveValidate, [0x6a, 0xff, 0x64, 0xa1, 0x00, 0x00, 0x00, 0x00]);
    validateBytes(VA.saveDispatch,
                  [0x8b, 0x44, 0x24, 0x04, 0x53, 0x56, 0x3d, 0x38, 0x27]);
    validateBytes(VA.characterCreate,
                  [0x83, 0xec, 0x20, 0x53, 0x55, 0x8b, 0xe9, 0x33, 0xdb, 0x56, 0x57]);
    validateBytes(VA.soundSetInit,
                  [0x83, 0xec, 0x08, 0x53, 0x55, 0x56, 0x57, 0x8b, 0xf9]);
    validateBytes(VA.soundPlay,
                  [0x81, 0xec, 0x98, 0x00, 0x00, 0x00, 0x8b, 0x84, 0x24]);
    validateBytes(VA.bufferStart,
                  [0x56, 0x8b, 0xf1, 0x6a, 0x00, 0x8b, 0x06]);
}

function isMonsterCode(code) {
    // Exact EN-SE's ordinary and scenario-specific monster code bands.  The
    // force hook still accepts an exact source outside these bands; they only
    // define what source=0 ("next monster") is allowed to consume.
    return (code >= 0xc742 && code <= 0xc83e) ||
           (code >= 0x18744 && code <= 0x1875d);
}

function installCharacterHooks() {
    // FUN_00419b00 is the exact EN-SE character constructor.  Its sixth stack
    // argument is the entity code, consumed before base stats, family assets,
    // AI and the sound set are initialized.  Rewriting it here produces a
    // complete target variant from a natural room spawn.
    Interceptor.attach(ap(VA.characterCreate), {
        onEnter() {
            this.owner = this.context.ecx;
            this.caller = this.returnAddress.sub(base).add(IMAGE_BASE).toString();
            this.originalCode = this.context.esp.add(0x18).readU32();
            this.param9 = this.context.esp.add(0x24).readU32();
            this.force = null;

            if (spawnForceQueue.length) {
                const request = spawnForceQueue[0];
                const sourceMatches = request.source === 0
                    ? isMonsterCode(this.originalCode)
                    : request.source === this.originalCode;
                if (sourceMatches) {
                    this.context.esp.add(0x18).writeU32(request.target);
                    this.force = {
                        request: request.id,
                        target: request.target,
                        source: request.source,
                    };
                    request.remaining -= 1;
                    if (request.remaining <= 0) spawnForceQueue.shift();
                    emit('character_force_applied', {
                        request: request.id,
                        owner: this.owner.toString(),
                        caller: this.caller,
                        original_code: this.originalCode,
                        target_code: request.target,
                        param9: this.param9,
                    });
                }
            }
        },
        onLeave() {
            let finalCode = null;
            try { finalCode = this.owner.add(0x1d4).readU32(); } catch (_) {}
            if (this.force !== null) {
                forcedOwners.set(this.owner.toString(), {
                    request: this.force.request,
                    original_code: this.originalCode,
                    target_code: this.force.target,
                    final_code: finalCode,
                });
            }
            if (this.force !== null || isMonsterCode(this.originalCode) ||
                (finalCode !== null && isMonsterCode(finalCode))) {
                emit('character_created', {
                    request: this.force === null ? null : this.force.request,
                    owner: this.owner.toString(),
                    caller: this.caller,
                    original_code: this.originalCode,
                    requested_code: this.force === null ? null : this.force.target,
                    final_code: finalCode,
                    param9: this.param9,
                    force_complete: this.force === null ? null : finalCode === this.force.target,
                });
            }
        },
    });
}

function compactRuns(rows) {
    if (!rows.length) return [];
    const out = [];
    let begin = rows[0];
    let previous = rows[0];
    for (let i = 1; i < rows.length; ++i) {
        const row = rows[i];
        if (row.index === previous.index + 1 && row.route === previous.route &&
            row.bank === previous.bank && row.gate === previous.gate) {
            previous = row;
            continue;
        }
        out.push({first: begin.index, last: previous.index, route: begin.route,
                  bank: begin.bank, gate: begin.gate});
        begin = previous = row;
    }
    out.push({first: begin.index, last: previous.index, route: begin.route,
              bank: begin.bank, gate: begin.gate});
    return out;
}

// This is the trace harness's proven injection model: put a separately-owned
// {id,timestamp,flag} record in the highest ring slot.  Do not rotate retail's
// pointer ring or guess at its producer state.
function injectInputRecord(manager, buttonId, engineNow, ordinal) {
    const rec = inputPool.add((inputPoolIndex % INPUT_POOL_RECORDS) * INPUT_RECORD_SIZE);
    inputPoolIndex += 1;
    rec.writeU32(buttonId >>> 0);
    rec.add(4).writeU32(engineNow >>> 0);
    rec.add(8).writeU32(1);
    const slot = RING_SLOTS - 1 - (ordinal % RING_SLOTS);
    manager.add(RING_BASE_OFF + slot * 4).writePointer(rec);
}

function installInputHooks() {
    inputPool = Memory.alloc(INPUT_POOL_RECORDS * INPUT_RECORD_SIZE);

    Interceptor.attach(ap(VA.inputPoll), {
        onEnter() {
            inputPollCount += 1;
            inputManager = this.context.ecx;
            const managerKey = inputManager.toString();
            if (managerKey !== activeInputManagerKey) {
                if (activeInputManagerKey !== null) {
                    emit('input_manager_left', {manager: activeInputManagerKey,
                                                buttons: activePollButtons});
                }
                activeInputManagerKey = managerKey;
                activePollButtons = {};
                emit('input_manager_entered', {manager: activeInputManagerKey});
            }
            let engineNow = 0;
            let buttonId = -1;
            try { engineNow = this.context.esp.add(4).readU32(); } catch (_) {}
            try { buttonId = this.context.esp.add(8).readU32(); } catch (_) {}
            lastEngineNow = engineNow;
            const buttonKey = String(buttonId);
            activePollButtons[buttonKey] = (activePollButtons[buttonKey] || 0) + 1;
            this.buttonId = buttonId;
            this.injected = false;

            // NativeFunction calls into the engine must run on its main thread.
            // inputPoll is a stable, validated main-loop safepoint; service at
            // most one synthetic monster sound per poll entry.
            if (soundTestQueue.length)
                serviceSoundTestQueue();

            if (queuedPresses.length) {
                const ids = queuedPresses.splice(0, queuedPresses.length);
                for (let i = 0; i < ids.length; ++i)
                    injectInputRecord(inputManager, ids[i], engineNow, i);
                this.injected = ids.indexOf(buttonId) >= 0;
                emit('input_injected', {ids: ids, now: engineNow,
                                        manager: inputManager.toString(),
                                        source: 'trace_ring'});
            }

            const at = polledPresses.indexOf(buttonId);
            if (at >= 0) {
                polledPresses.splice(at, 1);
                injectInputRecord(inputManager, buttonId, engineNow, 0);
                this.injected = true;
                emit('input_injected', {ids: [buttonId], now: engineNow,
                                        manager: inputManager.toString(),
                                        source: 'matching_retail_poll'});
            }

            // Prefer a freshly-drawn Flip.  Windowed paths that do not invoke
            // IDirectDrawSurface::Flip still get a deterministic engine-thread
            // fallback after one frame's worth of wall time.
            if (shotQueue.length && Date.now() - shotQueue[0].requestedAt >= 50)
                serviceShotQueue('input_poll');
        },
        onLeave(retval) {
            const accepted = retval.toUInt32();
            if (accepted !== 0 || this.injected) {
                emit('input_polled', {button: this.buttonId, accepted: accepted,
                                      injected: this.injected,
                                      manager: inputManager ? inputManager.toString() : null});
            }
        },
    });

    Interceptor.attach(ap(VA.keyDown), {
        onEnter() {
            try { this.scancode = this.context.esp.add(4).readU32() & 0xff; }
            catch (_) { this.scancode = -1; }
        },
        onLeave(retval) {
            if (this.scancode >= 0 && heldScancodes.has(this.scancode))
                retval.replace(ptr(0x80));
        },
    });

    // The main-game and Mystery Dungeon outer loops are different, but both
    // scan the same input record format before their later inputPoll(Abort)
    // calls.  Hook the exact per-frame boundary in each loop.  This distinction
    // is only input-driving machinery; monster combat/audio remains shared.
    function installGameplayInputBoundary(va, boundary) {
        Interceptor.attach(ap(va), {
          onEnter() {
            if (!queuedGameplayPressBatches.length || inputManager === null) return;
            const ids = queuedGameplayPressBatches.shift();
            for (let i = 0; i < ids.length; ++i)
                injectInputRecord(inputManager, ids[i], lastEngineNow, i);
            emit('gameplay_input_injected', {manager: inputManager.toString(),
                                             ids: ids, now: lastEngineNow,
                                             boundary: boundary,
                                             source: 'retail_gameplay_input_boundary'});
          },
        });
    }
    installGameplayInputBoundary(VA.mainGameInputBoundary, 'main_game');
    installGameplayInputBoundary(VA.mysteryDungeonInputBoundary, 'mystery_dungeon');

    // Generic menus (including Continue's save selector) scan the ring in
    // FUN_004378d0 *before* their later inputPoll(Abort) call.  Injecting at
    // inputPoll is therefore a stage late and retail clears the record before
    // the next controller pass.  Insert immediately on controller entry with
    // its own `now` argument.  The untouched controller interprets the IDs,
    // updates the real cursor/widget, and returns its normal result.
    Interceptor.attach(ap(VA.genericMenuController), {
        onEnter() {
            if (!queuedMenuPressBatches.length) return;
            const ids = queuedMenuPressBatches.shift();
            let engineNow = 0;
            try { engineNow = this.context.esp.add(8).readU32(); } catch (_) {}
            for (let i = 0; i < ids.length; ++i)
                injectInputRecord(this.context.ecx, ids[i], engineNow, i);
            emit('menu_input_injected', {manager: this.context.ecx.toString(),
                                         ids: ids, now: engineNow,
                                         source: 'retail_menu_controller'});
        },
    });
}

function tableDefinitions(key, action) {
    const out = [];
    for (let index = 0; index < 294; ++index) {
        const record = ap(VA.table).add(index * SOUND_RECORD_SIZE);
        const recordKey = record.readU32();
        if (recordKey === 0) break;
        const recordAction = record.add(4).readU16();
        if (recordKey !== (key >>> 0) || recordAction !== (action & 0xffff))
            continue;
        const managerIndex = record.add(8).readU16();
        const manager = readPtrSafe(ap(0x92afe0).add(managerIndex * 4));
        out.push({
            index: index,
            key: recordKey,
            action: recordAction,
            manager_index: managerIndex,
            manager: manager.toString(),
            se: record.add(0x0a).readU16(),
            se_param: record.add(0x0c).readS32(),
            copies_minus_two: record.add(0x14).readU16(),
            bank_selector: record.add(0x18).readS32(),
            voice: record.add(0x1c).readU16(),
            voice_param: record.add(0x20).readS32(),
        });
    }
    return out;
}

function directSoundStatus(wrapper) {
    if (wrapper.isNull()) return null;
    try {
        const buffer = readPtrSafe(wrapper);
        if (buffer.isNull()) return {wrapper: wrapper.toString(), buffer: '0x0'};
        const getStatusAddress = buffer.readPointer().add(9 * Process.pointerSize).readPointer();
        const getStatus = new NativeFunction(getStatusAddress, 'int',
                                             ['pointer', 'pointer'], 'stdcall');
        const statusOut = Memory.alloc(4);
        statusOut.writeU32(0);
        const hr = getStatus(buffer, statusOut);
        return {wrapper: wrapper.toString(), buffer: buffer.toString(),
                hresult: hr, flags: statusOut.readU32(),
                playing: (statusOut.readU32() & 1) !== 0};
    } catch (e) {
        return {wrapper: wrapper.toString(), error: '' + e};
    }
}

function describeResource(resource) {
    if (resource.isNull()) return null;
    try {
        const count = resource.readU16();
        const slots = readPtrSafe(resource.add(4));
        const buffers = [];
        if (!slots.isNull()) {
            for (let i = 0; i < Math.min(count, 16); ++i) {
                const object = readPtrSafe(slots.add(i * 8));
                buffers.push({
                    object: object.toString(),
                    busy: slots.add(i * 8 + 4).readU32(),
                    directsound: directSoundStatus(object),
                });
            }
        }
        return {
            address: resource.toString(),
            count: count,
            cursor: resource.add(2).readU16(),
            slots: slots.toString(),
            sound_device: readPtrSafe(resource.add(8)).toString(),
            clip: resource.add(0x0c).readU16(),
            bank: readPtrSafe(resource.add(0x10)).toString(),
            parameter: resource.add(0x14).readU16(),
            buffers: buffers,
        };
    } catch (e) {
        return {address: resource.toString(), error: '' + e};
    }
}

function describeSoundGroup(soundObject, action) {
    const result = {
        object: soundObject.toString(),
        action: action,
        group_count: 0,
        entries: [],
    };
    try {
        result.group_count = soundObject.add(4).readU16();
        if (action < 0 || action >= result.group_count) return result;
        const groups = readPtrSafe(soundObject);
        if (groups.isNull()) return result;
        const group = groups.add(action * 0x510);
        result.group = group.toString();
        result.priority = group.add(2).readU16();
        result.definition_count = group.add(6).readU16();
        for (let i = 0; i < Math.min(result.definition_count, 32); ++i) {
            const entry = group.add(8 + i * 0x28);
            const resource = readPtrSafe(entry.add(0x10));
            result.entries.push({
                index: i,
                parameter: entry.add(4).readS32(),
                voice_parameter: entry.add(8).readS32(),
                active: entry.add(0x0c).readU8(),
                resource: describeResource(resource),
                playing: readPtrSafe(entry.add(0x14)).toString(),
            });
        }
    } catch (e) {
        result.error = '' + e;
    }
    return result;
}

function hookDirectSoundPlay(buffer) {
    if (buffer.isNull()) return;
    let play;
    try { play = buffer.readPointer().add(12 * Process.pointerSize).readPointer(); }
    catch (_) { return; }
    const key = play.toString();
    if (hookedDirectSoundPlayFns.has(key)) return;
    hookedDirectSoundPlayFns.add(key);
    Interceptor.attach(play, {
        onEnter(args) {
            const call = activeSoundCalls.get(Process.getCurrentThreadId());
            if (call === undefined) return;
            this.call = call;
            this.buffer = args[0].toString();
            this.flags = args[3].toUInt32();
        },
        onLeave(retval) {
            if (this.call === undefined) return;
            emit('directsound_play', {
                request: this.call.request,
                key: this.call.key,
                action: this.call.action,
                sound_object: this.call.sound_object,
                buffer: this.buffer,
                flags: this.flags,
                hresult: retval.toInt32(),
            });
        },
    });
}

function installSoundRuntimeHooks() {
    soundSetInitFn = new NativeFunction(ap(VA.soundSetInit), 'void',
        ['pointer', 'uint32', 'int32', 'int32'], 'thiscall');
    soundPlayFn = new NativeFunction(ap(VA.soundPlay), 'int',
        ['pointer', 'uint32', 'int32', 'int32', 'int32', 'int32', 'int32'], 'thiscall');

    Interceptor.attach(ap(VA.soundSetInit), {
        onEnter() {
            this.owner = this.context.ecx;
            this.key = this.context.esp.add(4).readU32();
            this.x = this.context.esp.add(8).readS32();
            this.y = this.context.esp.add(12).readS32();
            try { this.entityCode = this.owner.add(0x1d4).readU32(); }
            catch (_) { this.entityCode = null; }
        },
        onLeave() {
            const soundObject = readPtrSafe(this.owner.add(0x15b78));
            if (soundObject.isNull()) return;
            const force = forcedOwners.get(this.owner.toString()) || null;
            const info = {owner: this.owner.toString(), key: this.key,
                          entity_code: this.entityCode, force: force,
                          x: this.x, y: this.y, synthetic_request: null};
            soundSetsByObject.set(soundObject.toString(), info);
            emit('sound_set_created', Object.assign({sound_object: soundObject.toString()}, info));
        },
    });

    Interceptor.attach(ap(VA.soundPlay), {
        onEnter() {
            const tid = Process.getCurrentThreadId();
            const objectKey = this.context.ecx.toString();
            const info = soundSetsByObject.get(objectKey) || {};
            const call = {
                request: info.synthetic_request === undefined ? null : info.synthetic_request,
                key: info.key === undefined ? null : info.key,
                owner: info.owner === undefined ? null : info.owner,
                entity_code: info.entity_code === undefined ? null : info.entity_code,
                force: info.force === undefined ? null : info.force,
                action: this.context.esp.add(4).readU32(),
                sound_object: objectKey,
                args: [],
            };
            for (let i = 0; i < 6; ++i)
                call.args.push(this.context.esp.add(4 + i * 4).readS32());
            this.tid = tid;
            this.call = call;
            activeSoundCalls.set(tid, call);
            emit('sound_play_enter', call);
        },
        onLeave(retval) {
            emit('sound_play_leave', Object.assign({}, this.call,
                                                   {result: retval.toInt32()}));
            activeSoundCalls.delete(this.tid);
        },
    });

    // This retail wrapper issues IDirectSoundBuffer::SetCurrentPosition(0)
    // followed by IDirectSoundBuffer::Play.  Hook the COM method dynamically so
    // a successful synthetic test is objective, rather than relying on hearing it.
    Interceptor.attach(ap(VA.bufferStart), {
        onEnter() {
            const call = activeSoundCalls.get(Process.getCurrentThreadId());
            if (call === undefined) return;
            const wrapper = this.context.ecx;
            const buffer = readPtrSafe(wrapper);
            hookDirectSoundPlay(buffer);
            emit('buffer_start', {
                request: call.request,
                key: call.key,
                action: call.action,
                sound_object: call.sound_object,
                wrapper: wrapper.toString(),
                buffer: buffer.toString(),
            });
        },
    });
}

function serviceSoundTestQueue() {
    const request = soundTestQueue.shift();
    if (request === undefined) return;
    try {
        // The constructor only touches these high owner fields. Memory.alloc is
        // zero-filled; retain the owner because retail owns the nested sound
        // object but has no knowledge of our outer test allocation.
        const owner = Memory.alloc(0x15b90);
        syntheticOwners.push(owner);
        soundSetInitFn(owner, request.key, request.x, request.y);
        const soundObject = readPtrSafe(owner.add(0x15b78));
        if (soundObject.isNull()) throw new Error('retail constructor returned a null sound object');
        const info = soundSetsByObject.get(soundObject.toString()) || {
            owner: owner.toString(), key: request.key, x: request.x, y: request.y,
        };
        info.synthetic_request = request.id;
        soundSetsByObject.set(soundObject.toString(), info);
        const before = describeSoundGroup(soundObject, request.action);
        emit('sound_test_constructed', {
            request: request.id,
            key: request.key,
            action: request.action,
            owner: owner.toString(),
            sound_object: soundObject.toString(),
            table: tableDefinitions(request.key, request.action),
            group: before,
        });
        const result = soundPlayFn(soundObject, request.action,
                                   request.args[0], request.args[1], request.args[2],
                                   request.args[3], request.args[4]);
        emit('sound_test_complete', {
            request: request.id,
            key: request.key,
            action: request.action,
            result: result,
            group_after: describeSoundGroup(soundObject, request.action),
        });
    } catch (e) {
        emit('sound_test_complete', {
            request: request.id,
            key: request.key,
            action: request.action,
            error: '' + e,
        });
    }
}

function installSaveDiagnostics() {
    Interceptor.attach(ap(VA.continueMenu), {
        onEnter() { this.begin = nowMs(); },
        onLeave(retval) {
            emit('continue_menu_leave', {result: retval.toUInt32(),
                                         elapsed_ms: nowMs() - this.begin});
        },
    });
    Interceptor.attach(ap(VA.saveRead), {
        onEnter() {
            this.caller = this.returnAddress.sub(base).add(IMAGE_BASE).toString();
            this.object = this.context.ecx.toString();
            this.args = [];
            for (let i = 0; i < 4; ++i) {
                try { this.args.push(this.context.esp.add(4 + i * 4).readU32()); }
                catch (_) { this.args.push(null); }
            }
        },
        onLeave(retval) {
            const result = retval.toUInt32();
            // Save-menu enumeration probes every possible slot. Keep the
            // evidence useful by reporting only successful reads.
            if (result !== 0)
                emit('save_read_leave', {caller: this.caller, object: this.object,
                                         args: this.args, result: result});
        },
    });
    Interceptor.attach(ap(VA.saveValidate), {
        onEnter() {
            this.caller = this.returnAddress.sub(base).add(IMAGE_BASE).toString();
            this.object = this.context.ecx.toString();
            this.args = [];
            for (let i = 0; i < 2; ++i) {
                try { this.args.push(this.context.esp.add(4 + i * 4).readU32()); }
                catch (_) { this.args.push(null); }
            }
        },
        onLeave(retval) {
            emit('save_validate_leave', {caller: this.caller, object: this.object,
                                         args: this.args, result: retval.toUInt32()});
        },
    });
    Interceptor.attach(ap(VA.saveDispatch), {
        onEnter() {
            this.selection = this.context.esp.add(4).readU32();
        },
        onLeave(retval) {
            emit('save_dispatch_leave', {selection: this.selection,
                                         result: retval.toUInt32()});
        },
    });
}

function vtableSlot(object, index) {
    return object.readPointer().add(index * Process.pointerSize).readPointer();
}

function ensureGdi() {
    if (gdi !== null) return true;
    const module = Process.findModuleByName('gdi32.dll');
    if (module === null) return false;
    function fn(name, ret, args) {
        const address = module.findExportByName(name);
        if (address === null) throw new Error('missing gdi32!' + name);
        return new NativeFunction(address, ret, args, 'stdcall');
    }
    gdi = {
        createCompatibleDC: fn('CreateCompatibleDC', 'pointer', ['pointer']),
        createDIBSection: fn('CreateDIBSection', 'pointer',
                            ['pointer', 'pointer', 'uint32', 'pointer', 'pointer', 'uint32']),
        selectObject: fn('SelectObject', 'pointer', ['pointer', 'pointer']),
        bitBlt: fn('BitBlt', 'int',
                   ['pointer', 'int', 'int', 'int', 'int', 'pointer', 'int', 'int', 'uint32']),
        deleteObject: fn('DeleteObject', 'int', ['pointer']),
        deleteDC: fn('DeleteDC', 'int', ['pointer']),
    };
    return true;
}

function surfaceDescription(surface) {
    try {
        const desc = Memory.alloc(DDSD2_SIZE);
        desc.writeU32(DDSD2_SIZE);
        const getDesc = new NativeFunction(vtableSlot(surface, V_SURF_GETSURFACEDESC),
                                           'uint32', ['pointer', 'pointer'], 'stdcall');
        const hr = getDesc(surface, desc);
        if (hr !== 0) return null;
        return {
            width: desc.add(DDSD2_WIDTH_OFF).readU32(),
            height: desc.add(DDSD2_HEIGHT_OFF).readU32(),
            caps: desc.add(DDSD2_CAPS_OFF).readU32(),
        };
    } catch (_) {
        return null;
    }
}

function captureSurface(entry, shotId, index, source) {
    if (!ensureGdi()) return false;
    const surface = entry.surface;
    const desc = surfaceDescription(surface);
    if (desc === null || desc.width < 160 || desc.height < 120 ||
        desc.width > 4096 || desc.height > 4096) return false;

    const getDC = new NativeFunction(vtableSlot(surface, V_SURF_GETDC),
                                     'uint32', ['pointer', 'pointer'], 'stdcall');
    const releaseDC = new NativeFunction(vtableSlot(surface, V_SURF_RELEASEDC),
                                         'uint32', ['pointer', 'pointer'], 'stdcall');
    const sourceDCOut = Memory.alloc(Process.pointerSize);
    sourceDCOut.writePointer(ptr(0));
    const hr = getDC(surface, sourceDCOut);
    if (hr !== 0) return false;
    const sourceDC = sourceDCOut.readPointer();

    let memoryDC = ptr(0);
    let bitmap = ptr(0);
    let oldObject = ptr(0);
    try {
        memoryDC = gdi.createCompatibleDC(sourceDC);
        if (memoryDC.isNull()) return false;
        const bmi = Memory.alloc(40);
        bmi.writeByteArray(new Uint8Array(40));
        bmi.writeU32(40);
        bmi.add(4).writeS32(desc.width);
        bmi.add(8).writeS32(-desc.height); // top-down DIB
        bmi.add(12).writeU16(1);
        bmi.add(14).writeU16(24);
        const bitsOut = Memory.alloc(Process.pointerSize);
        bitsOut.writePointer(ptr(0));
        bitmap = gdi.createDIBSection(memoryDC, bmi, DIB_RGB_COLORS,
                                      bitsOut, ptr(0), 0);
        if (bitmap.isNull()) return false;
        oldObject = gdi.selectObject(memoryDC, bitmap);
        if (gdi.bitBlt(memoryDC, 0, 0, desc.width, desc.height,
                       sourceDC, 0, 0, SRCCOPY) === 0) return false;
        const stride = (desc.width * 3 + 3) & ~3;
        const bytes = bitsOut.readPointer().readByteArray(stride * desc.height);
        emit('surface_frame', {
            shot: shotId,
            index: index,
            source: source,
            surface: surface.toString(),
            width: desc.width,
            height: desc.height,
            stride: stride,
            bpp: 24,
            caps: desc.caps,
            flip: flipCount,
        }, bytes);
        return true;
    } finally {
        try {
            if (!oldObject.isNull()) gdi.selectObject(memoryDC, oldObject);
            if (!bitmap.isNull()) gdi.deleteObject(bitmap);
            if (!memoryDC.isNull()) gdi.deleteDC(memoryDC);
            releaseDC(surface, sourceDC);
        } catch (_) {}
    }
}

function serviceShotQueue(source) {
    if (servicingShot || !shotQueue.length) return;
    servicingShot = true;
    const request = shotQueue.shift();
    let emitted = 0;
    const failures = [];
    try {
        if (screenSurface !== null) {
            try {
                if (captureSurface(screenSurface, request.id, emitted, source))
                    emitted += 1;
            } catch (e) {
                failures.push({surface: screenSurface.surface.toString(), error: '' + e});
            }
        }
    } finally {
        emit('shot_complete', {shot: request.id, source: source,
                               emitted: emitted, failures: failures,
                               surfaces_created: surfaceCreateCount,
                               screen_selected: screenSurface !== null});
        servicingShot = false;
    }
}

function hookSurfaceFlip(surface) {
    let flipFn;
    try { flipFn = vtableSlot(surface, V_SURF_FLIP); }
    catch (_) { return; }
    const key = flipFn.toString();
    if (hookedFlipFns.has(key)) return;
    hookedFlipFns.add(key);
    Interceptor.attach(flipFn, {
        onEnter() {
            flipCount += 1;
            if (shotQueue.length) serviceShotQueue('surface_flip');
        },
    });
}

function rememberSurface(surface, createDesc) {
    if (surface.isNull()) return;
    surfaceCreateCount += 1;
    if (screenSurface !== null) return;
    const live = surfaceDescription(surface);
    if (live === null || live.width !== 640 || live.height !== 480) return;
    try {
        // The first 640x480 offscreen surface is retail's persistent composed
        // screen canvas (live-verified against the rendered title menu).  Hold
        // only this one reference.  Retaining transient sprite surfaces changes
        // retail resource lifetimes and quickly exhausts memory in attract mode.
        const addRef = new NativeFunction(vtableSlot(surface, V_SURF_ADDREF),
                                          'uint32', ['pointer'], 'stdcall');
        addRef(surface);
    } catch (_) {}
    const entry = {key: surface.toString(), surface: surface,
                   create: createDesc, selected: live};
    screenSurface = entry;
    hookSurfaceFlip(surface);
    emit('ddraw_screen_surface', {surface: entry.key, create: createDesc,
                                  live: live, ordinal: surfaceCreateCount});
}

function readCreateDescription(desc) {
    try {
        if (desc.isNull()) return null;
        return {
            size: desc.readU32(),
            flags: desc.add(4).readU32(),
            height: desc.add(DDSD2_HEIGHT_OFF).readU32(),
            width: desc.add(DDSD2_WIDTH_OFF).readU32(),
            caps: desc.add(DDSD2_CAPS_OFF).readU32(),
        };
    } catch (_) {
        return null;
    }
}

function hookDirectDrawObject(object) {
    if (object.isNull()) return;
    const key = object.toString();
    if (ddrawObjects.indexOf(key) >= 0) return;
    ddrawObjects.push(key);
    const createSurface = vtableSlot(object, V_DDRAW_CREATE_SURFACE);
    const fnKey = createSurface.toString();
    if (!hookedCreateSurfaceFns.has(fnKey)) {
        hookedCreateSurfaceFns.add(fnKey);
        Interceptor.attach(createSurface, {
            onEnter(args) {
                this.desc = readCreateDescription(args[1]);
                this.out = args[2];
            },
            onLeave(retval) {
                if (retval.toUInt32() !== 0 || this.out.isNull()) return;
                try { rememberSurface(this.out.readPointer(), this.desc); }
                catch (e) { emit('ddraw_error', {where: 'CreateSurface/onLeave', error: '' + e}); }
            },
        });
    }
    emit('ddraw_object', {object: key, create_surface: createSurface.toString()});
}

function installDirectDrawHooks() {
    const module = Process.findModuleByName('ddraw.dll');
    if (module === null) throw new Error('ddraw.dll is not loaded');
    const create = module.findExportByName('DirectDrawCreateEx');
    if (create === null) throw new Error('ddraw!DirectDrawCreateEx is missing');
    Interceptor.attach(create, {
        onEnter(args) { this.out = args[1]; },
        onLeave(retval) {
            if (retval.toUInt32() !== 0 || this.out.isNull()) return;
            try { hookDirectDrawObject(this.out.readPointer()); }
            catch (e) { emit('ddraw_error', {where: 'DirectDrawCreateEx/onLeave', error: '' + e}); }
        },
    });
    emit('ddraw_hook_ready', {direct_draw_create_ex: create.toString()});
}

function installSoundHooks() {
    Interceptor.attach(ap(VA.bankLoad), {
        onEnter(args) {
            this.name = readUtf8Safe(args[0]);
            this.begin = nowMs();
            emit('bank_load_enter', {name: this.name,
                                     asset_mgr: this.context.ecx.toString()});
        },
        onLeave(retval) {
            emit('bank_load_leave', {name: this.name, result: retval.toString(),
                                     elapsed_ms: nowMs() - this.begin,
                                     voice_bank_global: bankValue().toString()});
        },
    });

    Interceptor.attach(ap(VA.registrarEnter), {
        onEnter() {
            registrarRun += 1;
            currentRegistrar = {run: registrarRun, begin: nowMs(), rows: []};
            emit('registrar_enter', {run: registrarRun,
                                     voice_bank: bankValue().toString(),
                                     gate: gateValue()});
        },
    });

    Interceptor.attach(ap(VA.registrarBankCheck), {
        onEnter() {
            if (currentRegistrar === null) return;
            const offset = this.context.eax.toUInt32();
            if (offset % SOUND_RECORD_SIZE !== 0) return;
            const index = offset / SOUND_RECORD_SIZE;
            const record = ap(VA.table).add(offset);
            const bank = bankValue();
            const gate = gateValue();
            const voice = record.add(0x1c).readU16();
            let route = 'se';
            if (!bank.isNull() && gate === 0)
                route = (voice === 0 || voice === 0x7fff) ? 'skip-unvoiced' : 'voice';
            currentRegistrar.rows.push({
                index: index,
                key: record.readU32(),
                action: record.add(4).readU16(),
                se: record.add(0x0a).readU16(),
                voice: voice,
                bank: bank.toString(),
                gate: gate,
                route: route,
            });
        },
    });

    Interceptor.attach(ap(VA.registrarExit), {
        onEnter() {
            if (currentRegistrar === null) return;
            const skipped = currentRegistrar.rows.filter(row => row.route === 'skip-unvoiced');
            emit('registrar_exit', {
                run: currentRegistrar.run,
                elapsed_ms: nowMs() - currentRegistrar.begin,
                voice_bank: bankValue().toString(),
                gate: gateValue(),
                checked_rows: currentRegistrar.rows.length,
                route_runs: compactRuns(currentRegistrar.rows),
                skipped: skipped.map(row => ({index: row.index, key: row.key,
                                              action: row.action, se: row.se,
                                              voice: row.voice})),
            });
            currentRegistrar = null;
        },
    });

    bankPoll = setInterval(function () {
        const bank = bankValue().toString();
        if (bank !== lastBank) {
            emit('voice_bank_changed', {before: lastBank, after: bank,
                                        gate: gateValue()});
            lastBank = bank;
        }
    }, 1);
}

function typedRead(va, type) {
    const p = ap(va);
    switch (type || 'u32') {
        case 'u8': return p.readU8();
        case 'i8': return p.readS8();
        case 'u16': return p.readU16();
        case 'i16': return p.readS16();
        case 'i32': return p.readS32();
        case 'ptr': return p.readPointer().toString();
        default: return p.readU32();
    }
}

function typedWrite(va, type, value) {
    const p = ap(va);
    switch (type || 'u32') {
        case 'u8': case 'i8': p.writeU8(value & 0xff); break;
        case 'u16': case 'i16': p.writeU16(value & 0xffff); break;
        default: p.writeU32(value >>> 0); break;
    }
}

rpc.exports = {
    init() {
        startedMs = Date.now();
        base = Process.mainModule.base;
        lastBank = bankValue().toString();
        validateBuild();
        installDirectDrawHooks();
        installSoundHooks();
        installCharacterHooks();
        installSoundRuntimeHooks();
        installInputHooks();
        installSaveDiagnostics();
        Interceptor.flush();
        emit('repro_ready', {module: Process.mainModule.name,
                             base: base.toString(), voice_bank: lastBank});
        return true;
    },
    status() {
        return {
            ms: nowMs(),
            registrar_run: registrarRun,
            voice_bank: bankValue().toString(),
            gate: gateValue(),
            input_manager: inputManager ? inputManager.toString() : null,
            input_polls: inputPollCount,
            poll_buttons: activePollButtons,
            queued_presses: queuedPresses.slice(),
            polled_presses: polledPresses.slice(),
            menu_press_batches: queuedMenuPressBatches.slice(),
            gameplay_press_batches: queuedGameplayPressBatches.slice(),
            held_scancodes: Array.from(heldScancodes),
            sound_test_queue: soundTestQueue.slice(),
            synthetic_sound_owners: syntheticOwners.length,
            spawn_force_queue: spawnForceQueue.map(request => Object.assign({}, request)),
            forced_owners: Array.from(forcedOwners.entries()).map(entry => ({
                owner: entry[0], force: entry[1],
            })),
            ddraw_objects: ddrawObjects.slice(),
            surface_create_count: surfaceCreateCount,
            screen_surface: screenSurface === null ? null : {
                surface: screenSurface.key,
                create: screenSurface.create,
                live: surfaceDescription(screenSurface.surface),
            },
            flips: flipCount,
            pending_shots: shotQueue.map(request => request.id),
        };
    },
    press(ids) {
        const values = Array.isArray(ids) ? ids : [ids];
        for (let i = 0; i < values.length; ++i) queuedPresses.push(values[i] | 0);
        return queuedPresses.slice();
    },
    presswhen(ids) {
        const values = Array.isArray(ids) ? ids : [ids];
        for (let i = 0; i < values.length; ++i) polledPresses.push(values[i] | 0);
        return polledPresses.slice();
    },
    clearpresses() {
        queuedPresses = [];
        polledPresses = [];
        queuedMenuPressBatches = [];
        queuedGameplayPressBatches = [];
        return true;
    },
    menuconfirm() {
        queuedMenuPressBatches.push([0x25]);
        return queuedMenuPressBatches.length;
    },
    menupress(ids) {
        const values = (Array.isArray(ids) ? ids : [ids]).map(value => value | 0);
        queuedMenuPressBatches.push(values);
        return queuedMenuPressBatches.length;
    },
    gameplayconfirm() {
        // EN-SE's live menu/dialogue mapping uses 0x25.  The older JP retail
        // build used 0x24; do not import that build's ID into this probe.
        queuedGameplayPressBatches.push([0x25]);
        return queuedGameplayPressBatches.length;
    },
    gameplaypress(ids) {
        const values = (Array.isArray(ids) ? ids : [ids]).map(value => value | 0);
        queuedGameplayPressBatches.push(values);
        return queuedGameplayPressBatches.length;
    },
    hold(scancodes) {
        heldScancodes = new Set((scancodes || []).map(value => value & 0xff));
        return Array.from(heldScancodes);
    },
    forcespawn(target, source, count) {
        const targetCode = target >>> 0;
        const sourceCode = source >>> 0;
        const total = Math.max(1, Math.min(count | 0, 32));
        if (!isMonsterCode(targetCode))
            throw new Error('target is not an exact EN-SE monster code');
        spawnForceSequence += 1;
        spawnForceQueue.push({id: spawnForceSequence, target: targetCode,
                              source: sourceCode, remaining: total});
        return spawnForceSequence;
    },
    clearspawnforces() {
        spawnForceQueue = [];
        return true;
    },
    soundtest(key, action, args) {
        soundTestSequence += 1;
        const values = Array.isArray(args) ? args.slice(0, 5) : [];
        while (values.length < 5) values.push(0);
        soundTestQueue.push({
            id: soundTestSequence,
            key: key >>> 0,
            action: action >>> 0,
            x: 0,
            y: 0,
            args: values.map(value => value | 0),
        });
        return soundTestSequence;
    },
    shot() {
        shotSequence += 1;
        shotQueue.push({id: shotSequence, requestedAt: Date.now()});
        return shotSequence;
    },
    read(va, type) {
        return typedRead(va >>> 0, type);
    },
    write(va, type, value) {
        typedWrite(va >>> 0, type, value);
        return true;
    },
    stop() {
        if (bankPoll !== null) clearInterval(bankPoll);
        bankPoll = null;
        return true;
    },
};
