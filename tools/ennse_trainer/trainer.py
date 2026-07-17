#!/usr/bin/env python3
"""EN-SE trainer — LLM-operable manipulation of a live retail sotes_en.exe.

Attach-mode side-car: run tools/ennse_voice/repro_probe.py (or the plain game)
first, then this. Composes with the probe (which owns spawn/input/screenshots).

Protocol: line-delimited JSON on stdin, one JSON response per line on stdout.
Agent events (actor_created, sound_test_done, freeze_dead) stream interleaved.

Commands (see README.md for the LLM playbook):
  {"cmd":"read","addr":"0x12345678","type":"u32|u16|u8|i32|f32|ptr"}
  {"cmd":"write","addr":...,"value":...,"type":...}
  {"cmd":"hunt","value":90}              -> all aligned RW dwords == value
  {"cmd":"winnow","addrs":[...],"value":89}
  {"cmd":"lock","name":"hp","addr":...,"value":90,"type":"u32"}   (50ms rewrite)
  {"cmd":"unlock","name":"hp"} / {"cmd":"locks"}
  {"cmd":"snap","name":"arche","addr":...,"size":88976}
  {"cmd":"snapdiff","name":"arche","mode":"dec|inc|any"}
  {"cmd":"actors","monsters":true} / {"cmd":"actorclear"}
  {"cmd":"forcespawn","target":51083,"source":0,"count":4} / {"cmd":"forceclear"}
  {"cmd":"soundtest","key":51081,"action":12}
  {"cmd":"quit"}
"""

from __future__ import annotations

import json
import os
import select
import sys
from pathlib import Path

import frida

REMOTE = os.environ.get("OPENSUMMONERS_FRIDA_REMOTE",
                        os.environ.get("OPENRECET_FRIDA_REMOTE",
                                       "cutestation.soy:27042"))
AGENT = Path(__file__).with_name("trainer_agent.js")


def _num(v):
    return int(v, 0) if isinstance(v, str) else int(v)


def main() -> int:
    device = frida.get_device_manager().add_remote_device(REMOTE)
    procs = [p for p in device.enumerate_processes()
             if p.name.lower() == "sotes_en.exe"]
    if not procs:
        print(json.dumps({"kind": "error", "error": "sotes_en.exe not running"}),
              flush=True)
        return 1
    session = device.attach(procs[0].pid)
    script = session.create_script(AGENT.read_text(encoding="utf-8"))

    def on_message(message, _data) -> None:
        if message.get("type") == "error":
            print(json.dumps({"kind": "agent_error",
                              "description": message.get("description")}), flush=True)
        else:
            print(json.dumps(message.get("payload") or {}), flush=True)

    script.on("message", on_message)
    script.load()
    script.exports_sync.init()
    print(json.dumps({"kind": "ready", "pid": procs[0].pid}), flush=True)

    x = script.exports_sync
    while True:
        readable, _, _ = select.select([sys.stdin], [], [], 0.25)
        if not readable:
            continue
        line = sys.stdin.readline()
        if not line:
            break
        try:
            req = json.loads(line)
            cmd = req.get("cmd")
            if cmd == "read":
                res = {"ok": True, "value": x.read(_num(req["addr"]), req.get("type"))}
            elif cmd == "write":
                res = {"ok": True, "written": x.write(_num(req["addr"]), req["value"],
                                                      req.get("type"))}
            elif cmd == "readblock":
                res = {"ok": True, "values": x.readblock(_num(req["addr"]), req["size"])}
            elif cmd == "hunt":
                res = {"ok": True, "addrs": x.hunt(req["value"])}
            elif cmd == "winnow":
                res = {"ok": True, "addrs": x.winnow(req["addrs"], req["value"])}
            elif cmd == "lock":
                res = {"ok": True, "locked": x.freezeset(req["name"], _num(req["addr"]),
                                                         req["value"], req.get("type"))}
            elif cmd == "unlock":
                res = {"ok": True, "unlocked": x.freezedel(req["name"])}
            elif cmd == "locks":
                res = {"ok": True, "locks": x.freezelist()}
            elif cmd == "snap":
                res = {"ok": True, "snapped": x.snap(req["name"], _num(req["addr"]),
                                                     req.get("size", 88976))}
            elif cmd == "snapdiff":
                res = {"ok": True, "diff": x.snapdiff(req["name"],
                                                      req.get("mode", "any"))}
            elif cmd == "actors":
                res = {"ok": True, "actors": x.actors(bool(req.get("monsters")))}
            elif cmd == "actorclear":
                res = {"ok": True, "cleared": x.actorclear()}
            elif cmd == "forcespawn":
                res = {"ok": True, "request": x.forcespawn(_num(req["target"]),
                                                           _num(req.get("source", 0)),
                                                           req.get("count", 1))}
            elif cmd == "forceclear":
                res = {"ok": True, "cleared": x.forceclear()}
            elif cmd == "player":
                res = {"ok": True, "player": x.player()}
            elif cmd == "playerset":
                res = {"ok": True, "actor": x.playerset(
                    _num(req["actor"]) if req.get("actor") else None)}
            elif cmd == "teleport":
                res = {"ok": True, "player": x.teleport(
                    req.get("x"), req.get("y"), bool(req.get("relative")))}
            elif cmd == "setstat":
                res = {"ok": True, "player": x.setstat(req["which"], req["value"],
                                                       bool(req.get("lock")))}
            elif cmd == "tpnearest":
                res = {"ok": True, "result": x.tpnearest(bool(req.get("monsters", True)))}
            elif cmd == "soundtest":
                res = {"ok": True, "request": x.soundtest(_num(req["key"]),
                                                          _num(req["action"]))}
            elif cmd == "quit":
                print(json.dumps({"ok": True, "bye": True}), flush=True)
                break
            else:
                res = {"ok": False, "error": f"unknown cmd {cmd!r}"}
        except Exception as exc:  # noqa: BLE001
            res = {"ok": False, "error": repr(exc)}
        print(json.dumps(res), flush=True)
    try:
        session.detach()
    except Exception:  # noqa: BLE001
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
