#!/usr/bin/env python3
r"""rd — drive Fortune Summoners (retail, or the standalone port) live, at savestate speed, and turn what you did
into a verified trace.

One command per call; the run directory is the state. Behind a run sits a LIVE rhost process (the game on the headless
devices, `rhost --serve`): it steps frames on demand, reads memory, takes in-process savestates, renders pictures from
its own memory and runs the exploration search. That process is only a cache — a run is its MOVIE (every key level since
boot); a dead session is rebuilt by replaying it.

    rd new R --from fresh | SCENARIO[@F] | RUN[:CK|@F] | end:SCENARIO | save:DIGEST   [--standalone]
    rd play R "X . X*3 .*30 R*40 ent"  [--until EXPR] [--avoid EXPR] [--watch EXPR,..] [--every N]
    rd tap R "ent" [--n 40] [--gap 8] [--until EXPR] [--text]                 press, release, repeat
    rd status R | rd text R | rd eval R EXPR.. [--at CK | --at-frame F] | rd ram R list | read VA [N] | dwords VA [N] | str VA | define NAME .. | mark | diff
    rd scan R --from F [--to T] --watch EXPR,.. [--every N] [--changes] [--where EXPR] [--at-run RUN] [--fresh]
    rd ck R save NAME | load NAME | list        rd undo R        rd rewind R --to F
    rd search R --goal EXPR [--avoid EXPR] [--cell TUPLE] [--actions SPEC ..] [--score EXPR] [--hold N] [--apply|--apply-best] [-j N]
    rd look R [--at F,F..]      rd strip R --from F --to F [--every N]    pictures (headless: from the session's memory)
    rd verify R [--real]        rd check R TASK.md      rd lint TASK.md --on RUN..
    rd endsave R                the user\ dir (statue saves) this run left → save:DIGEST for the next run
    rd file R NAME              tests/scenarios/NAME (then: rhost.py corpus --only NAME --bless / --check --hybrid / --standalone)
    rd accept R NAME [--task T] check → verify --real → file → bless → hybrid + standalone check → report into docs/findings/probes/
    rd stop R | rd ls

Method and the full reference: tools/drive/README.md; the one-page card for a driving agent: tools/drive/DRIVER.md.
"""
import argparse
import ast
import math
import fcntl
import glob
import gzip
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "rhost"))
import rhost as RH  # noqa: E402
sys.path.insert(0, os.path.join(ROOT, "tools", "bunker"))
try:
    import bunker as BK  # noqa: E402
except ImportError:                                     # bunker support is optional (tools/bunker)
    BK = None

RUNS = os.environ.get("RD_RUNS", "/mnt/ephemeral/opensummoners/drive/runs")
SESS = os.path.join(RH.WINE_ROOT, "runs", "drive")          # live sessions' rhost run dirs (C:\oss-rhost\runs\drive\..)
VARS = os.path.join(HERE, "vars.json")
# (openrecet skips a proven-pure render function in live sessions; SotES has none proven yet: RD_SKIP_FN stays empty
# until `rhost.py skip-check` passes the corpus for a candidate)
SKIP_FN = os.environ.get("RD_SKIP_FN", "")
SKIP_MASK = os.environ.get("RD_SKIP_MASK", "")
SKIP_MASK_V1 = ""


def skip_args():
    return ["--skip-fn", SKIP_FN, "--mask-range", SKIP_MASK] if SKIP_FN else []


SCEN_SAVES = os.path.join(RH.SCEN, "_saves")

# ------------------------------------------------------------------ keys / input specs
# bit → DirectInput scancode: the SAME table as tools/rhost/rh_serve.c KEYMAP (a movie's masks are these bits)
KEYS = [("U", 0xc8), ("D", 0xd0), ("L", 0xcb), ("R", 0xcd), ("X", 0x2d), ("C", 0x2e), ("Z", 0x2c), ("V", 0x2f), ("F", 0x21),
        ("ent", 0x1c), ("esc", 0x01), ("bk", 0x0e), ("f1", 0x3b), ("f2", 0x3c), ("f3", 0x3d), ("f4", 0x3e), ("f5", 0x3f),
        ("f6", 0x40), ("n0", 0x52), ("n1", 0x4f), ("n2", 0x50), ("n3", 0x51), ("n4", 0x4b), ("n5", 0x4c), ("n6", 0x4d),
        ("n7", 0x47), ("n8", 0x48), ("n9", 0x49), ("pu", 0xc9), ("pd", 0xd1), ("home", 0xc7), ("sp", 0x39)]
BTN = {k: 1 << i for i, (k, _) in enumerate(KEYS)}             # name → mask bit
LETTERS = "UDLRXCZVF"                                           # one-letter keys: combine as RX, UC, ...
DIK_OF = {1 << i: dik for i, (_, dik) in enumerate(KEYS)}
BIT_OF_DIK = {dik: 1 << i for i, (_, dik) in enumerate(KEYS)}


def mask_name(m):
    if not m:
        return "."
    letters = "".join(k for k in LETTERS if m & BTN[k])
    longs = [k for k, _ in KEYS if len(k) > 1 and m & BTN[k]]
    return "+".join(([letters] if letters else []) + longs)


def mask_keys(m):
    """mask → sorted DIK list (a trace line's "keys")"""
    return sorted(DIK_OF[b] for b in DIK_OF if m & b)


def keys_mask(keys):
    """DIK list → mask (a key outside KEYS: SystemExit — the movie could not hold it)"""
    m = 0
    for k in keys:
        if k not in BIT_OF_DIK:
            raise SystemExit(f"key 0x{k:02x} is not in the rd key table (tools/drive/drive.py KEYS = rh_serve.c KEYMAP)")
        m |= BIT_OF_DIK[k]
    return m


def parse_token(t):
    """one token → [(mask, n, esc)]: X  RX*8  .*30  ent  R+ent  f1  0x30*4   (esc is the Esc KEY here: a plain key)"""
    m = re.fullmatch(r"([A-Za-z0-9.+]*?)(?:\*(\d+))?", t)
    if not m or not m.group(1):
        raise SystemExit(f"bad input token {t!r} (e.g. X  RX*8  .*30  ent  R+ent  f1  0x30*4)")
    body, n = m.group(1), int(m.group(2) or 1)
    if body.lower().startswith("0x"):
        return [(int(body, 16), n, False)]
    if body == ".":
        return [(0, n, False)]
    mask = 0
    for part in body.split("+"):
        if part in BTN and len(part) > 1:
            mask |= BTN[part]
        elif part.upper() == part and part and all(ch in LETTERS for ch in part):
            for ch in part:
                mask |= BTN[ch]
        else:
            raise SystemExit(f"bad key {part!r} in {t!r} (letters {' '.join(LETTERS)} combine: RX; named: "
                             f"{' '.join(k for k, _ in KEYS if len(k) > 1)}; join with +: R+ent; . = none)")
    return [(mask, n, False)]


def parse_spec(spec):
    """"A . (A .*7)x12 R*40" → [(mask, n, esc)]"""
    spec = spec.strip().strip("\"'`").strip()
    out, i = [], 0
    while i < len(spec):
        if spec[i].isspace():
            i += 1
        elif spec[i] == "{":                    # {EXPR}*N: the mask is the expression's value, read before each tick
            j = spec.find("}", i)
            if j < 0:
                raise SystemExit("unclosed { in the input spec: {pad(ex - px, ez - pz) | A}*12")
            m = re.match(r"\*(\d+)", spec[j + 1:])
            out.append((("expr", spec[i + 1:j]), int(m.group(1)) if m else 1, False))
            i = j + 1 + (m.end() if m else 0)
        elif spec[i] == "(":
            j, depth, brace = i, 0, 0            # the group's own ")": nested groups and {EXPR} parentheses skipped
            while j < len(spec):
                ch = spec[j]
                brace += (ch == "{") - (ch == "}")
                if not brace:
                    depth += (ch == "(") - (ch == ")")
                    if depth == 0:
                        break
                j += 1
            if j >= len(spec):
                raise SystemExit("unclosed ( in the input spec")
            m = re.match(r"x(\d+)", spec[j + 1:])
            if not m:
                raise SystemExit("a group needs a repeat count: (A .*7)x12")
            out += parse_spec(spec[i + 1:j]) * int(m.group(1))
            i = j + 1 + m.end()
        else:
            j = i
            while j < len(spec) and not spec[j].isspace() and spec[j] != "(":
                j += 1
            out += parse_token(spec[i:j])
            i = j
    return out


def spec_to_run(items, prog=None):
    """[(mask, n, esc)] → (segments "m:n,m:n", [esc ticks], total); prog(expr) → program id, for {EXPR} masks"""
    segs, escs, t = [], [], 0
    for mask, n, esc in items:
        if n <= 0:
            continue
        if esc:
            escs.append(t + 1)
        if segs and segs[-1][0] == mask:
            segs[-1][1] += n
        else:
            segs.append([mask, n])
        t += n
    def one(m):
        if isinstance(m, tuple):
            if prog is None:
                raise SystemExit("{EXPR} masks need a live session (play / tap / search actions)")
            return f"p{prog(m[1])}"
        return f"{m:x}"
    return ",".join(f"{one(m)}:{n}" for m, n in segs), escs, t


def segs_to_spec(path):
    """server path "10:2,0:8" → "A*2 .*8\""""
    out = []
    for s in filter(None, path.split(",")):
        m, n = s.split(":")
        out.append(mask_name(int(m, 16)) + (f"*{n}" if n != "1" else ""))
    return " ".join(out)


# ------------------------------------------------------------------ expressions → postfix programs
# rh_live.c limits: a command line is split into at most 600 tokens (`prog ID` + 598: a longer program is cut
# SILENTLY), the VM stack holds 64 values, NPROG 256 program ids (the client wraps its ids at 250), `eval` takes
# 64 ids, `run s=` 32, `run esc=` / `rng=` 16 each, a command line 64 KiB.
MAX_PROG_TOKENS = 598
MAX_STACK = 64
EVAL_CHUNK = 48                   # programs defined + evaluated per `eval` (< 64 ids, far from the id wrap at 250)
MAX_SAMPLED = 32
MAX_RUN_OPS = 16
MAX_LINE = 60000

# actors(COND): COND per entity slot with these names bound to the slot's fields. SotES's entity table is not mapped
# into rd yet (src/world_layout.h has the layouts): ACTOR_BASE None → actors() says so.
ACTOR_BASE, ACTOR_SIZE, ACTOR_SLOTS = None, 0, 0
ACTOR_FIELDS = {}

_ARITY = {"r8": 1, "r16": 1, "r32": 1, "s8": 1, "s16": 1, "s32": 1, "f32": 1, "f64": 1, "~": 1, "not": 1, "neg": 1, "abs": 1,
          "sqrt": 1, "sin": 1, "cos": 1, "sel": 3, "frame": 0, "rng": 0}
_BINARY = {"+", "-", "*", "/", "//", "%", "&", "|", "^", "<<", ">>", "<", "<=", ">", ">=", "==", "!=", "and", "or", "min", "max",
           "atan2", "hypot", "pow"}


def stack_depth(tokens):
    """the VM stack's high-water mark for a postfix program"""
    sp = hi = 0
    for t in tokens:
        if t in _BINARY:
            sp -= 1
        elif t in _ARITY:
            sp += 1 - _ARITY[t]
        else:
            sp += 1
        hi = max(hi, sp)
    return hi


def prog_check(tokens, what="expression"):
    """a program the server runs as written: ≤ MAX_PROG_TOKENS tokens (more are cut without an error), ≤ MAX_STACK deep"""
    if len(tokens) > MAX_PROG_TOKENS:
        raise SystemExit(f"{what}: {len(tokens)} postfix tokens > {MAX_PROG_TOKENS} (the live server's limit). Values read by "
                         "eval / --watch / scan may be sums of programs (a top-level `+`, sum(..), actors(..)); a stop condition "
                         "(--until, --avoid, --goal, --score, a task step) is one program: narrow it — actors(COND, LO, HI) over fewer slots")
    if stack_depth(tokens) > MAX_STACK:
        raise SystemExit(f"{what}: needs a stack of {stack_depth(tokens)} values > {MAX_STACK}: nest less (a + (b + (c + ..))) → a + b + c ..)")
    return tokens


def split_top(s, sep=","):
    """"a, hypot(b, c), (d, e)" → ["a", "hypot(b, c)", "(d, e)"]: split on SEP outside brackets only; empty items dropped"""
    out, depth, cur = [], 0, []
    for ch in s or "":
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == sep and depth == 0:
            out.append("".join(cur)); cur = []
        else:
            cur.append(ch)
    out.append("".join(cur))
    return [x.strip() for x in out if x.strip()]


TYPES ={"u8": "r8", "s8": "s8", "u16": "r16", "s16": "s16", "u32": "r32", "s32": "s32", "i32": "s32", "f32": "f32", "f64": "f64"}
BINOP = {ast.Add: "+", ast.Sub: "-", ast.Mult: "*", ast.Div: "/", ast.FloorDiv: "//", ast.Mod: "%", ast.BitAnd: "&",
         ast.BitOr: "|", ast.BitXor: "^", ast.LShift: "<<", ast.RShift: ">>"}
CMPOP = {ast.Lt: "<", ast.LtE: "<=", ast.Gt: ">", ast.GtE: ">=", ast.Eq: "==", ast.NotEq: "!="}


class Vars:
    """named state: tools/drive/vars.json (shared, committed) + <run>/vars.json (the run's own)"""

    def __init__(self, run_dir=None, also=()):
        """ALSO: other runs' dirs whose names are visible too (below this run's own)"""
        self.defs, self.local_path, self._slot = {}, run_dir and os.path.join(run_dir, "vars.json"), None
        for p in [VARS] + [os.path.join(d, "vars.json") for d in also] + [self.local_path]:
            if p and os.path.exists(p):
                self.defs.update({k: v for k, v in json.load(open(p)).items() if not k.startswith("_")})

    def define(self, name, d, shared=False):
        p = VARS if shared else self.local_path
        cur = json.load(open(p)) if os.path.exists(p) else {}
        cur[name] = d
        json.dump(cur, open(p + ".tmp", "w"), indent=1)
        os.replace(p + ".tmp", p)
        self.defs[name] = d

    def compile(self, expr, depth=0):
        """infix (Python syntax over named vars) → list of postfix tokens"""
        try:
            tree = ast.parse(expr.strip(), mode="eval").body
        except SyntaxError as e:
            raise SystemExit(f"bad expression {expr!r}: {e.msg}")
        return self._c(tree, depth, expr)

    def _c(self, n, depth, src):
        c = lambda x: self._c(x, depth, src)
        if isinstance(n, ast.Constant) and isinstance(n.value, (int, float, bool)):
            return [repr(float(n.value)) if isinstance(n.value, float) else str(int(n.value))]
        if isinstance(n, ast.Name):
            if self._slot is not None and n.id in self._slot:
                return list(self._slot[n.id])
            if n.id in ("frame", "rng"):
                return [n.id]
            if n.id == "pi" and "pi" not in self.defs:
                return [repr(math.pi)]
            if n.id not in self.defs and n.id.isupper() and all(ch in BTN for ch in n.id):
                return [str(sum({BTN[ch] for ch in n.id}))]         # a button mask: A, UR, B
            if n.id not in self.defs:
                raise SystemExit(f"unknown name {n.id!r} in {src!r} — `rd ram R list`, or define it (`rd ram R define`)")
            if depth > 12:
                raise SystemExit(f"variable definitions recurse at {n.id!r}")
            d = self.defs[n.id]
            if "expr" in d:
                return self.compile(d["expr"], depth + 1)
            return [str(int(str(d["addr"]), 16)), TYPES[d.get("type", "s32")]]
        if isinstance(n, ast.Call) and isinstance(n.func, ast.Name):
            f = n.func.id
            if f in TYPES and len(n.args) == 1:
                return c(n.args[0]) + [TYPES[f]]
            if f in ("min", "max") and len(n.args) >= 2:
                out = c(n.args[0])
                for a in n.args[1:]:
                    out += c(a) + [f]
                return out
            if f == "sum":
                return self._fold([c(e) for e in self._sum_args(n, src)])
            if f == "actors":
                return self._fold(self._actor_terms(n, depth, src))
            if f == "abs" and len(n.args) == 1:
                return c(n.args[0]) + ["abs"]
            if f == "int" and len(n.args) == 1:
                return c(n.args[0]) + ["1", "//"]
            if f in ("sqrt", "sin", "cos") and len(n.args) == 1:
                return c(n.args[0]) + [f]
            if f in ("atan2", "hypot", "pow") and len(n.args) == 2:
                return c(n.args[0]) + c(n.args[1]) + [f]
            if f == "key" and n.args and all(isinstance(x, ast.Constant) and isinstance(x.value, str) for x in n.args):
                m = 0
                for x in n.args:
                    m |= parse_token(x.value)[0][0]
                return [str(m)]
            if f == "toward" and len(n.args) in (1, 2):
                # the arrow that walks along dx (screen x: R = +x); (dx, dead): 0 within ±dead
                dx = f"({ast.unparse(n.args[0])})"
                dead = ast.unparse(n.args[1]) if len(n.args) == 2 else "0"
                return self.compile(f"({BTN['R']} if {dx} > {dead} else ({BTN['L']} if {dx} < -({dead}) else 0))", depth + 1)
            if f == "pad" and len(n.args) == 2:
                # the arrows (8 directions) along the screen vector (dx, dy): R = +x, D = +y (menus, cursors)
                x, y = (f"({ast.unparse(a)})" for a in n.args)
                return self.compile(f"(({BTN['R']} if {x} > 0 else {BTN['L']}) if abs({x}) > 0.41421356 * abs({y}) else 0) + "
                                    f"(({BTN['D']} if {y} > 0 else {BTN['U']}) if abs({y}) > 0.41421356 * abs({x}) else 0)", depth + 1)
            raise SystemExit(f"unknown function {f}() in {src!r} (u8 s8 u16 s16 u32 s32 f32 f64 min max abs int sqrt sin cos atan2 hypot pow "
                             "key toward pad sum actors)")
        if isinstance(n, ast.BinOp) and isinstance(n.op, ast.Pow):
            return c(n.left) + c(n.right) + ["pow"]
        if isinstance(n, ast.BinOp) and type(n.op) in BINOP:
            return c(n.left) + c(n.right) + [BINOP[type(n.op)]]
        if isinstance(n, ast.UnaryOp):
            if isinstance(n.op, ast.USub):
                return c(n.operand) + ["neg"]
            if isinstance(n.op, ast.Not):
                return c(n.operand) + ["not"]
            if isinstance(n.op, ast.Invert):
                return c(n.operand) + ["~"]
            if isinstance(n.op, ast.UAdd):
                return c(n.operand)
        if isinstance(n, ast.BoolOp):
            out = c(n.values[0])
            for v in n.values[1:]:
                out += c(v) + ["and" if isinstance(n.op, ast.And) else "or"]
            return out
        if isinstance(n, ast.IfExp):
            return c(n.test) + c(n.body) + c(n.orelse) + ["sel"]
        if isinstance(n, ast.Compare):
            out, left = None, n.left
            for op, right in zip(n.ops, n.comparators):
                if isinstance(op, (ast.In, ast.NotIn)):
                    if not isinstance(right, (ast.Tuple, ast.List)) or not right.elts:
                        raise SystemExit(f"`in` needs a tuple of values in {src!r}")
                    one = None
                    for e in right.elts:
                        t = c(left) + c(e) + ["=="]
                        one = t if one is None else one + t + ["or"]
                    if isinstance(op, ast.NotIn):
                        one += ["not"]
                elif type(op) in CMPOP:
                    one = c(left) + c(right) + [CMPOP[type(op)]]
                else:
                    raise SystemExit(f"unsupported comparison in {src!r}")
                out = one if out is None else out + one + ["and"]
                left = right
            return out
        raise SystemExit(f"unsupported syntax in {src!r}: {ast.dump(n)[:80]}")

    @staticmethod
    def _fold(terms):
        """[t1, t2, t3] → t1 t2 + t3 + (a left fold: the stack stays 2 deep)"""
        out = list(terms[0]) if terms else ["0"]
        for t in terms[1:]:
            out += t + ["+"]
        return out

    @staticmethod
    def _sum_args(n, src):
        """sum(e1, e2, ..) or sum((e1, e2, ..)) / sum([..]) → the element nodes"""
        if n.keywords:
            raise SystemExit(f"sum() takes no keywords in {src!r}")
        if len(n.args) == 1 and isinstance(n.args[0], (ast.Tuple, ast.List)):
            return n.args[0].elts
        if len(n.args) == 1 and not isinstance(n.args[0], ast.Constant):
            raise SystemExit(f"sum() takes the terms themselves: sum(a, b, c) or sum((a, b, c)) — no comprehensions ({src!r})")
        return n.args

    def _actor_terms(self, n, depth, src):
        """actors(COND[, LO, HI]) → one program per slot LO..HI (default 0..63): 1 when the slot is in use (U != 0, unless
        COND names U itself) and COND holds; COND reads the slot as T U HP X Y Z YAW ST TK SUB POW UNHIT STAG AGE AE4, and I
        (the slot index)"""
        if ACTOR_BASE is None:
            raise SystemExit("actors() needs SotES's entity table in rd (ACTOR_BASE in tools/drive/drive.py) — not mapped yet")
        if self._slot is not None:
            raise SystemExit(f"actors() cannot nest ({src!r})")
        if not 1 <= len(n.args) <= 3 or n.keywords:
            raise SystemExit(f"actors(COND) / actors(COND, HI) / actors(COND, LO, HI) in {src!r}")
        try:
            rng_ = [int(ast.literal_eval(a)) for a in n.args[1:]]
        except ValueError:
            raise SystemExit(f"actors(): the slot range must be numbers ({src!r})")
        lo, hi = (0, ACTOR_SLOTS - 1) if not rng_ else (0, rng_[0]) if len(rng_) == 1 else tuple(rng_)
        if not 0 <= lo <= hi < 128:
            raise SystemExit(f"actors(): slots {lo}..{hi} outside 0..127 ({src!r})")
        cond = n.args[0]
        names_u = any(isinstance(x, ast.Name) and x.id == "U" for x in ast.walk(cond))
        terms = []
        for i in range(lo, hi + 1):
            base = ACTOR_BASE + i * ACTOR_SIZE
            self._slot = {k: [str(base + off), TYPES[ty]] for k, (off, ty) in ACTOR_FIELDS.items()}
            self._slot["I"] = [str(i)]
            try:
                t = self._c(cond, depth, src)
                if not names_u:
                    t = self._slot["U"] + t + ["and"]                 # and → 0 / 1
                elif not (isinstance(cond, (ast.Compare, ast.BoolOp)) or (isinstance(cond, ast.UnaryOp) and isinstance(cond.op, ast.Not))):
                    t = t + ["0", "!="]                              # a count, not a sum of values
            finally:
                self._slot = None
            terms.append(t)
        return terms

    def _terms(self, n, depth, src):
        """the top-level summands of an expression: [(sign, tokens)] (a + b - c, sum(..), actors(..), a name defined as one)"""
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub)):
            right = self._terms(n.right, depth, src)
            if isinstance(n.op, ast.Sub):
                right = [(-s, t) for s, t in right]
            return self._terms(n.left, depth, src) + right
        if isinstance(n, ast.Call) and isinstance(n.func, ast.Name) and n.func.id == "sum":
            return [x for e in self._sum_args(n, src) for x in self._terms(e, depth, src)]
        if isinstance(n, ast.Call) and isinstance(n.func, ast.Name) and n.func.id == "actors":
            return [(1, t) for t in self._actor_terms(n, depth, src)]
        if isinstance(n, ast.Name) and n.id in self.defs and "expr" in self.defs[n.id] and depth <= 12:
            d = self.defs[n.id]["expr"]
            try:
                return self._terms(ast.parse(d.strip(), mode="eval").body, depth + 1, d)
            except SyntaxError as e:
                raise SystemExit(f"bad expression {d!r}: {e.msg}")
        return [(1, self._c(n, depth, src))]

    def compile_parts(self, expr):
        """infix → [tokens, ..] whose values SUM to the expression's: one program when it fits the server's limits, else
        its top-level summands packed into programs that do (for values read by eval / --watch / scan)"""
        toks = self.compile(expr)
        if len(toks) <= MAX_PROG_TOKENS and stack_depth(toks) <= MAX_STACK:
            return [toks]
        terms = self._terms(ast.parse(expr.strip(), mode="eval").body, 0, expr)
        if len(terms) == 1:
            prog_check(toks, repr(expr))                     # not a sum: nothing to split
        parts, cur = [], None
        for s, t in terms:
            if len(t) + 1 > MAX_PROG_TOKENS or stack_depth(t) + 1 > MAX_STACK:
                prog_check(["0"] + t + ["+"], f"a summand of {expr!r}")
            if cur is not None and len(cur) + len(t) + 1 <= MAX_PROG_TOKENS:
                cur += t + (["+"] if s > 0 else ["-"])
            else:
                if cur is not None:
                    parts.append(cur)
                cur = t + ([] if s > 0 else ["neg"])
        parts.append(cur)
        return parts

    def program(self, expr, what=None):
        """infix → ONE program (a stop condition, a goal, a mask): SystemExit when it exceeds the server's limits"""
        return prog_check(self.compile(expr), what or repr(expr))

    def compile_tuple(self, expr):
        """"(scene, px//1, pz//1)" → [tokens, tokens, ..]"""
        tree = ast.parse(expr.strip(), mode="eval").body
        elts = tree.elts if isinstance(tree, ast.Tuple) else [tree]
        return [self._c(e, 0, expr) for e in elts]


# ------------------------------------------------------------------ the movie
class Movie:
    """every key level since boot: ops (frame, "m", mask) = from frame on, exactly mask's keys are held; + the start save
    (a user\\ dir: the game's statue saves + config.dat), + the edition"""

    def __init__(self, save=None, ops=None, frame=0, edition="enold", standalone=False):
        self.save, self.ops, self.frame, self.edition, self.standalone = save, ops or [], frame, edition, standalone

    @staticmethod
    def from_server(text, save, edition="enold"):
        kv = dict(t.split("=", 1) for t in text.split()[1:] if "=" in t)
        ops = []
        for o in filter(None, kv.get("ops", "").split(",")):
            f, v = o[1:].split(":")
            ops.append((int(f), "m", int(v, 16)))
        return Movie(save, ops, int(kv["frame"]), edition)   # (the caller restores .standalone)

    @staticmethod
    def load(path):
        """an rd movie or any rhost trace (frame-keyed DIK levels)"""
        mv = Movie()
        for line in open(path):
            line = line.strip()
            if not line.startswith("{"):
                continue
            d = json.loads(line)
            if "savefile" in d:
                mv.save = None if d["savefile"] == "@fresh" else os.path.normpath(os.path.join(os.path.dirname(path), d["savefile"]))
            elif "meta" in d:
                mv.edition = d["meta"].get("edition", mv.edition)
                mv.standalone = bool(d["meta"].get("standalone", False))
            elif "frame" in d and "keys" in d:
                m = keys_mask(d["keys"])
                if not mv.ops or mv.ops[-1][2] != m:
                    mv.ops.append((int(d["frame"]), "m", m))
            elif "end" in d:
                mv.frame = int(d["end"])
        if not mv.frame and mv.ops:
            mv.frame = max(o[0] for o in mv.ops)
        return mv

    def write(self, path, comment="", rel_to=None):
        """an rhost trace: replays to exactly self.frame (`--frames`); {"end": F} marks the movie's length"""
        with open(path + ".tmp", "w") as f:
            f.write(f"# rhost trace (rd movie{': ' + comment if comment else ''}): DIK levels per flip-frame, from boot\n")
            f.write(json.dumps({"meta": dict({"edition": self.edition, "rd": True}, **({"standalone": True} if self.standalone else {}))}) + "\n")
            if self.save:
                f.write(json.dumps({"savefile": os.path.relpath(self.save, rel_to or os.path.dirname(path))}) + "\n")
            for fr, k, v in self.ops:
                if fr > self.frame:
                    break
                f.write(json.dumps({"frame": fr, "keys": mask_keys(v)}) + "\n")
            f.write(json.dumps({"frame": self.frame, "keys": mask_keys(self.mask_at(self.frame))}) + "\n")
            f.write(json.dumps({"end": self.frame}) + "\n")
        os.replace(path + ".tmp", path)

    def truncated(self, frame):
        return Movie(self.save, [o for o in self.ops if o[0] <= frame], min(frame, self.frame), self.edition, self.standalone)

    def mask_at(self, frame):
        m = 0
        for fr, k, v in self.ops:
            if fr > frame:
                break
            m = v
        return m

    def run_args(self, lo, hi):
        """the inputs of frames lo..hi-1 (the frames a `run` from frame lo plays) as a `run` command's segments"""
        segs = []
        cur, at = self.mask_at(lo), lo
        for fr, k, v in self.ops:
            if fr <= lo or fr >= hi:
                continue
            if fr > at:
                segs.append((cur, fr - at)); at = fr
            cur = v
        if hi > at:
            segs.append((cur, hi - at))
        return ",".join(f"{m:x}:{n}" for m, n in segs)

    def chunks(self, lo, hi, max_ticks=4000):
        """frames lo..hi as [(a, b)] windows whose `run` line fits the server (< MAX_LINE)"""
        out, a = [], lo
        while a < hi:
            b = min(hi, a + max_ticks)
            while b > a + 1 and len(self.run_args(a, b)) >= MAX_LINE:
                b = a + (b - a) // 2
            out.append((a, b))
            a = b
        return out


# ------------------------------------------------------------------ a live rhost process
class SessionEnded(Exception):
    """the rhost process went away mid-command: the game exited (EXIT on the title, a quit dialog) or crashed"""


class Live:
    def __init__(self, port, timeout=3600):
        last = None
        for _ in range(150):
            try:
                self.s = socket.create_connection(("127.0.0.1", port), timeout=5)
                break
            except OSError as e:
                last = e
                time.sleep(0.1)
        else:
            raise OSError(f"cannot connect to the live session on port {port}: {last}")
        self.s.settimeout(timeout)
        self.s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.f = self.s.makefile("rwb")
        self.nprog = 0

    def cmd(self, line, check=True):
        self.f.write(line.encode() + b"\n")
        self.f.flush()
        r = self.f.readline().decode(errors="replace").rstrip("\n")
        if not r:
            raise SessionEnded()
        if check and not r.startswith("ok"):
            raise SystemExit(f"live: {r}\n  (command: {line[:200]})")
        return r

    @staticmethod
    def kv(r):
        return dict(t.split("=", 1) for t in r.split()[1:] if "=" in t)

    def prog(self, tokens):
        prog_check(tokens, "program " + " ".join(tokens[:8]) + (" …" if len(tokens) > 8 else ""))
        if self.nprog >= 250:
            self.nprog = 0
        pid = self.nprog
        self.nprog += 1
        self.cmd(f"prog {pid} " + " ".join(tokens))
        return pid

    def close(self):
        try:
            self.s.close()
        except OSError:
            pass


def num(v):
    return int(v) if v == int(v) and abs(v) < 1e15 else v


def eval_exprs(lv, V, exprs):
    """the expressions' values now; batched (EVAL_CHUNK programs per `eval`), an over-long sum split (compile_parts)"""
    flat = [(i, p) for i, e in enumerate(exprs) for p in V.compile_parts(e)]
    vals = [0.0] * len(exprs)
    for c in range(0, len(flat), EVAL_CHUNK):
        batch = flat[c:c + EVAL_CHUNK]
        ids = [lv.prog(p) for _, p in batch]
        for (i, _), x in zip(batch, Live.kv(lv.cmd("eval " + ",".join(map(str, ids))))["v"].split(":")):
            vals[i] += float(x)
    return [num(v) for v in vals]


def sampled_cols(lv, V, exprs, room=MAX_SAMPLED):
    """programs for sampled columns (`run s=`) → (ids, slices): a column's value = the sum of its slice of the row"""
    ids, slices = [], []
    for e in exprs:
        parts = V.compile_parts(e)
        slices.append((len(ids), len(ids) + len(parts)))
        ids += [None] * len(parts)
        if len(ids) > room:
            raise SystemExit(f"at most {room} sampled programs per run (the live server's limit; a long actors()/sum column "
                             f"takes several): {len(ids)}+ for {len(exprs)} columns — split the watch list over two calls")
        for k, p in enumerate(parts):
            ids[slices[-1][0] + k] = lv.prog(p)
    return ids, slices


def fold_row(vals, slices):
    return [num(sum(vals[a:b])) for a, b in slices]


def parse_samples(text):
    """`run … e=1` samples "f:v:v;f:v:v" → [(frame, [float])]"""
    rows = []
    for row in filter(None, (text or "").split(";")):
        p = row.split(":")
        rows.append((int(p[0]), [float(x) for x in p[1:]]))
    return rows


def wine_env():
    """live sessions run without an X display, in the headless prefix (rhost.py wine_prefix: why it is separate)"""
    return RH.wine_env(True)


def staged_exe(host="wine"):
    """build if stale, stage the build content-addressed (wine root, or bin\\<sha> on the Windows box / bunker) → its Windows path"""
    if host == "bunker":
        return f"{RH.WROOT}\\bin\\{BK.stage()}\\rhost.exe"
    if host == "win":
        if RH.rhost_stale():
            RH.cmd_build()
        sha = RH.file_sha12(os.path.join(RH.BUILD, "rhost.exe"))
        if sha not in _win_staged:
            rc, out = win_ps(f"if (Test-Path 'C:\\oss-rhost\\bin\\{sha}\\{RH.WEXE}') {{ 'HAVE' }} else {{ New-Item -ItemType Directory -Force "
                             f"'C:\\oss-rhost\\bin\\{sha}' | Out-Null; 'NEED' }}")
            if "NEED" in out:
                RH.scp(os.path.join(RH.BUILD, "rhost.exe"), f"{RH.WIN}:C:/oss-rhost/bin/{sha}/{RH.WEXE}.tmp")
                win_ps(f"Move-Item -Force 'C:\\oss-rhost\\bin\\{sha}\\{RH.WEXE}.tmp' 'C:\\oss-rhost\\bin\\{sha}\\{RH.WEXE}'")
            _win_staged.add(sha)
        return f"C:\\oss-rhost\\bin\\{sha}\\{RH.WEXE}"
    return RH.bin_dir_local() + "\\rhost.exe"


_win_staged = set()


RD_HOST = os.environ.get("RD_HOST", "auto")
HOSTS = ("win", "bunker", "wine")                      # the auto order
WIN_MAX_SESSIONS = int(os.environ.get("RD_WIN_MAX_SESSIONS", "8"))
BUNKER_MAX_SESSIONS = int(os.environ.get("RD_BUNKER_MAX_SESSIONS", "6"))
BUNKER_FLOOR_MB = int(os.environ.get("RD_BUNKER_MEM_FLOOR_MB", "1024"))
ADMIT_WAIT = {"win": float(os.environ.get("RD_WIN_ADMIT_WAIT", "60")), "bunker": float(os.environ.get("RD_BUNKER_ADMIT_WAIT", "30"))}
WIN_SESS = "C:\\oss-rhost\\runs\\drive"
_up, _up_lock = {"wine": True}, threading.Lock()


def host_up(h):
    """does host H answer (once per process)"""
    with _up_lock:
        if h not in _up:
            _up[h] = win_up(timeout=3) if h == "win" else (BK is not None and BK.up(timeout=3))
        return _up[h]


def pick_host():
    """the first host of the order that answers (RD_HOST=win|bunker|wine: that one)"""
    if RD_HOST not in ("auto",) + HOSTS:
        raise SystemExit(f"RD_HOST={RD_HOST!r}: auto | win | bunker | wine")
    return RD_HOST if RD_HOST != "auto" else next(h for h in HOSTS if host_up(h))


def local_listening(port):
    """is something LISTENing on 127.0.0.1:PORT here (the ssh forward is up)?"""
    for f in ("/proc/net/tcp", "/proc/net/tcp6"):
        try:
            for l in open(f).readlines()[1:]:
                p = l.split()
                if p[3] == "0A" and int(p[1].rsplit(":", 1)[1], 16) == port:
                    return True
        except OSError:
            pass
    return False


def free_local_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def win_ps(ps, timeout=600, host=None):
    """a PowerShell command on the Windows box → (rc, stdout); transient ssh failures retried (RH.ssh_win's rule)"""
    for attempt in range(4):
        r = subprocess.run(["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=8", host or RH.WIN, ps], text=True, capture_output=True, timeout=timeout)
        if r.returncode == 255 and RH.SSH_TRANSIENT.search(r.stderr) and attempt < 3:
            time.sleep(2 + 3 * attempt)
            continue
        return r.returncode, r.stdout
    return r.returncode, r.stdout


class NotAdmitted(SystemExit):
    """the Windows box's admission gave no slot in time"""


def win_spawn(name, ld, rhost_args, wait, tag):
    """ld (a local dir with the session's files) → C:\\oss-rhost\\runs\\drive\\NAME; start rhost there detached (WMI: outside
    sshd's job), admitted by the box's gate (≤ queue\\procs.txt oss-rhost processes, RAM floor, one Global\\oss-rhost-admit
    mutex, ≤ RD_WIN_MAX_SESSIONS rd sessions) → (windows pid, its serve port)"""
    exe = staged_exe("win")
    rd = f"{WIN_SESS}\\{name}"
    dst = f"{RH.WIN}:{WIN_SESS.replace(chr(92), '/')}/"
    if subprocess.run(["scp", "-q", "-r", ld, dst], capture_output=True).returncode != 0:
        win_ps(f"New-Item -ItemType Directory -Force '{WIN_SESS}' | Out-Null")
        RH.scp(ld, dst, recursive=True)
    shutil.rmtree(ld, ignore_errors=True)
    cl = " ".join([exe] + [f'"{x}"' if " " in x else x for x in rhost_args])
    ps = ("$ErrorActionPreference = 'Stop'\n"
          f"$rd = '{rd}'; $cl = '{cl.replace(chr(39), chr(39) * 2)}'\n"
          "$m = New-Object System.Threading.Mutex($false, 'Global\\oss-rhost-admit')\n"
          f"$deadline = (Get-Date).AddSeconds({wait}); $wpid = 0\n"
          "while ($true) {\n"
          "  $cap = 12; try { $cap = [int](Get-Content 'C:\\oss-rhost\\queue\\procs.txt' -TotalCount 1 -ErrorAction Stop) } catch {}\n"
          "  try { [void]$m.WaitOne() } catch [System.Threading.AbandonedMutexException] {}\n"
          f"  $ours = @(Get-CimInstance Win32_Process -Filter \"Name='{RH.WEXE}'\" | Where-Object {{ $_.ExecutablePath -like 'C:\\oss-rhost\\*' }})\n"
          "  $n = $ours.Count; $mine = @($ours | Where-Object { $_.CommandLine -like '*\\runs\\drive\\*' }).Count\n"
          "  $free = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory\n"
          f"  if ($mine -lt {WIN_MAX_SESSIONS} -and ($n -eq 0 -or ($n -lt $cap -and $free -gt 1500000))) {{\n"
          "    $r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = $cl; CurrentDirectory = $rd }\n"
          "    $wpid = $r.ProcessId\n"
          "    try { if ((Get-Content 'C:\\oss-rhost\\queue\\priority.txt' -TotalCount 1 -ErrorAction Stop).Trim() -eq 'low') { (Get-Process -Id $wpid).PriorityClass = 'BelowNormal' } } catch {}\n"
          "    Start-Sleep -Milliseconds 300; $m.ReleaseMutex(); break }\n"
          "  $m.ReleaseMutex()\n"
          f"  if ((Get-Date) -gt $deadline) {{ \"NOADMIT $n oss-rhost processes (cap $cap), $mine rd sessions (cap {WIN_MAX_SESSIONS}), $([int]($free / 1024)) MB free (floor 1465)\"; Remove-Item $rd -Recurse -Force -ErrorAction SilentlyContinue; exit 0 }}\n"
          "  Start-Sleep -Milliseconds 500 }\n"
          "$t0 = Get-Date\n"
          "while (!(Test-Path \"$rd\\serve.port\")) {\n"
          "  if (!(Get-Process -Id $wpid -ErrorAction SilentlyContinue) -or ((Get-Date) - $t0).TotalSeconds -gt 600) {\n"
          "    \"NOSTART pid $wpid\"; if (Test-Path \"$rd\\rhost.log\") { Get-Content \"$rd\\rhost.log\" -Tail 8 }; exit 0 }\n"
          "  Start-Sleep -Milliseconds 50 }\n"
          "\"LIVE $wpid \" + (Get-Content \"$rd\\serve.port\" -TotalCount 1)\n")
    rc, out = win_ps(ps, timeout=wait + 900)
    m = re.search(r"^LIVE (\d+) (\d+) ", out, re.M)
    if not m:
        if "NOADMIT" in out:
            raise NotAdmitted(f"live session {tag} on {RH.WIN}: not admitted — " + out.split("NOADMIT", 1)[1].strip() +
                             ". Stop idle sessions (`./rd ls`, `./rd stop R`) or wait.")
        raise SystemExit(f"the live session did not start on {RH.WIN} ({rd}, rc {rc})\n{out[-800:]}")
    return int(m.group(1)), int(m.group(2))


def win_forward(wpid, wport):
    """an ssh local forward to the session's port, alive exactly as long as the Windows process → (ssh pid, local port)"""
    for attempt in range(4):
        lp = free_local_port()
        f = subprocess.Popen(["ssh", "-o", "BatchMode=yes", "-o", "ExitOnForwardFailure=yes", "-o", "ServerAliveInterval=30",
                              "-o", "ServerAliveCountMax=4", "-L", f"127.0.0.1:{lp}:127.0.0.1:{wport}", RH.WIN,
                              f"Wait-Process -Id {wpid} -ErrorAction SilentlyContinue"],
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        t0 = time.time()
        while f.poll() is None and time.time() - t0 < 30:
            if local_listening(lp):
                return f.pid, lp
            time.sleep(0.05)
        try:
            os.killpg(f.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        f.wait()
        time.sleep(1 + 2 * attempt)
    raise SystemExit(f"no ssh forward to the live session (pid {wpid}, port {wport}) on {RH.WIN}")


def spawn_live(tag, movie, upto=None, idle=1800, wait=300, host=None, src_trace=None, limit=None, standalone=False):
    """a live rhost (headless, --serve) on HOST (pick_host()): replays MOVIE to frame UPTO first — or SRC_TRACE up to
    LIMIT — → the session dict {host, pid, port, rundir[, fwd, wport]}: `port` is always a local port (wine: the session's
    own; win / bunker: the ssh forward's). Admitted by the host's gate: WAIT seconds for a slot, else SystemExit"""
    if host is None and RD_HOST == "auto":
        for h in ("win", "bunker"):
            if not host_up(h):
                continue
            try:
                return spawn_live(tag, movie, upto, idle, min(wait, ADMIT_WAIT[h]), h, src_trace, limit, standalone)
            except NotAdmitted as e:
                print(f"[{e} — trying the next host]")
                wait = max(0, wait - ADMIT_WAIT[h])
        host = "wine"
    host = host or pick_host()
    standalone = standalone or getattr(movie, "standalone", False)
    name = f"{tag}-{os.getpid()}-{int(time.time() * 1000) % 10 ** 9}"
    if host in ("win", "bunker"):
        ld = os.path.join(RUNS, ".winstage", name)
        os.makedirs(ld)
        win = f"{WIN_SESS}\\{name}"
    else:
        exe = staged_exe("wine")
        os.makedirs(SESS, exist_ok=True)
        ld = os.path.join(SESS, name)
        os.makedirs(ld)
        win = f"{RH.WROOT}\\runs\\drive\\" + name
    ed = movie.edition or "enold"
    if os.path.exists(RH.volatile_file(ed)):
        shutil.copy(RH.volatile_file(ed), os.path.join(ld, "volatile.txt"))
    open(os.path.join(ld, "hooks.txt"), "w").write("")
    if movie.save:
        RH.stage_save(movie.save, ld)
    if src_trace:
        shutil.copy(src_trace, os.path.join(ld, "trace.jsonl"))
    elif (movie.frame if upto is None else upto) > 0:
        movie.truncated(movie.frame if upto is None else upto).write(os.path.join(ld, "trace.jsonl"))
    args = session_args(movie, win, upto, src_trace, limit, idle, standalone) + ["--hook", f"@{win}\\hooks.txt"]
    if host == "win":
        try:
            try:
                wpid, wport = win_spawn(name, ld + "", args, wait, tag) if False else win_spawn_retry(name, ld, args, wait, tag)
            except NotAdmitted:
                raise
        finally:
            shutil.rmtree(ld, ignore_errors=True)
        s = {"host": "win", "ssh": RH.WIN, "pid": wpid, "wport": wport, "rundir": win}
        try:
            s["fwd"], s["port"] = win_forward(wpid, wport)
        except BaseException:
            kill_session(s)
            raise
        return s
    if host == "bunker":
        try:
            files = {}
            for root, _, fs in os.walk(ld):
                for f in fs:
                    rel = os.path.relpath(os.path.join(root, f), ld)
                    files[rel] = open(os.path.join(root, f), "rb").read()
        finally:
            shutil.rmtree(ld, ignore_errors=True)
        exe = staged_exe("bunker")
        r = BK.spawn_session(name, files, exe, args, wait, BUNKER_MAX_SESSIONS, BUNKER_FLOOR_MB)
        if r[0] == "NOADMIT":
            raise NotAdmitted(f"live session {tag} on bunker: not admitted — {r[1]}. Stop idle sessions (`./rd ls`, `./rd stop R`) or wait.")
        if r[0] != "LIVE":
            raise SystemExit(f"the live session did not start on bunker ({win})\n{r[1]}")
        s = {"host": "bunker", "ssh": BK.HOST, "pid": r[1], "wport": r[2], "rundir": win, "name": name}
        try:
            for attempt in range(4):
                lp = free_local_port()
                if BK.forward(lp, r[2]):
                    s["port"] = lp
                    break
                time.sleep(0.5 + attempt)
            else:
                raise SystemExit(f"no forward to the live session on bunker (pid {r[1]}, port {r[2]})")
        except BaseException:
            kill_session(s)
            raise
        return s
    try:
        with RH.admit_local(wait=wait, what=f"live session {tag}"):
            p = subprocess.Popen(["wine", exe] + args, stdout=open(os.path.join(ld, "stdout.txt"), "w"), stderr=subprocess.DEVNULL,
                                 env=wine_env(), start_new_session=True, cwd=ld)
    except SystemExit:
        shutil.rmtree(ld, ignore_errors=True)
        raise
    pf = os.path.join(ld, "serve.port")
    t0 = time.time()
    while not os.path.exists(pf):
        if p.poll() is not None or time.time() - t0 > 900:
            tail = open(os.path.join(ld, "rhost.log"), errors="replace").read()[-600:] if os.path.exists(os.path.join(ld, "rhost.log")) else ""
            raise SystemExit(f"the live session did not start ({ld})\n{tail}")
        time.sleep(0.05)
    return {"host": "wine", "pid": p.pid, "port": int(open(pf).read().split()[0]), "rundir": ld}


def win_spawn_retry(name, ld, args, wait, tag):
    """win_spawn, once more when the first attempt failed without a word (first-time staging on the box)"""
    keep = ld + ".keep"
    shutil.copytree(ld, keep)
    try:
        try:
            return win_spawn(name, ld, args, wait, tag)
        except NotAdmitted:
            raise
        except SystemExit as e:
            if "rc 0" in str(e) or not re.search(r"rc \d+\)\n\s*$", str(e)):
                raise
            print("[the session did not start on the first try (no output): once more]")
            shutil.copytree(keep, ld)
            return win_spawn(name, ld, args, wait, tag)
    finally:
        shutil.rmtree(keep, ignore_errors=True)


def kill_session(s):
    """end OUR session (never by name): wine — its process group; win — the ssh forward, then the Windows process by PID
    after checking its command line is this session's; drop its run dir"""
    if not s:
        return
    if s.get("host") == "bunker":
        if s.get("port") and s.get("wport"):
            BK.forward(s["port"], s["wport"], cancel=True)
        if s.get("pid") and s.get("name") and s.get("rundir") == f"{WIN_SESS}\\{s['name']}":
            BK.kill_session(s["pid"], s["name"])
        return
    if s.get("host", "wine") == "win":
        if s.get("fwd"):
            try:
                os.killpg(s["fwd"], signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
        rd = s.get("rundir", "")
        if not rd.startswith(WIN_SESS + "\\") or not s.get("pid"):
            return
        win_ps(host=s.get("ssh"), ps=f"$p = Get-CimInstance Win32_Process -Filter 'ProcessId={int(s['pid'])}'; "
               f"if ($p -and $p.Name -eq '{RH.WEXE}' -and $p.CommandLine -like '*{rd}*') {{ Stop-Process -Id {int(s['pid'])} -Force; "
               f"Wait-Process -Id {int(s['pid'])} -Timeout 10 -ErrorAction SilentlyContinue }}; "
               f"Remove-Item '{rd}' -Recurse -Force -ErrorAction SilentlyContinue; exit 0", timeout=120)
        return
    try:
        os.killpg(s["pid"], signal.SIGKILL)
    except (ProcessLookupError, PermissionError):
        pass
    rundir = s.get("rundir")
    if rundir and os.path.realpath(rundir).startswith(os.path.realpath(SESS) + os.sep):
        shutil.rmtree(rundir, ignore_errors=True)


def pid_alive(pid):
    try:
        os.kill(pid, 0)
        return True
    except (ProcessLookupError, PermissionError):
        return False


def sess_alive(s):
    """win: the forward lives exactly as long as the Windows process (its remote command waits on it); bunker: the
    forward on the master is listening and the session's process is alive there (one round trip on the master)"""
    if s and s.get("host") == "bunker":
        return bool(s.get("port")) and local_listening(s["port"]) and BK.session_alive(s["pid"], s["name"])
    return bool(s) and pid_alive(s["fwd"] if s.get("host", "wine") == "win" else s["pid"])


def sess_file(s, f, dst):
    """copy file (or dir, e.g. user) F of the session's run dir to DST → True when it exists"""
    if s.get("host") == "bunker":
        return BK.session_get(s.get("name", ""), f, dst)      # file or dir (user)
    if s.get("host", "wine") == "win":
        return subprocess.run(["scp", "-q", "-r", f"{s.get('ssh', RH.WIN)}:{s['rundir'].replace(chr(92), '/')}/{f}", dst],
                              capture_output=True).returncode == 0 and os.path.exists(dst)
    src = os.path.join(s["rundir"], f)
    if not os.path.exists(src):
        return False
    if os.path.isdir(src):
        shutil.copytree(src, dst)
    else:
        shutil.copy(src, dst)
    return True


# ------------------------------------------------------------------ a run
class Run:
    def __init__(self, name, create=False, lock=True):
        """lock=False: a read-only view (movie, checkpoints, names) that never writes the run and does not wait for it"""
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", name):
            raise SystemExit(f"bad run name {name!r}")
        self.name, self.dir = name, os.path.join(RUNS, name)
        if create:
            os.makedirs(self.dir, exist_ok=True)
        elif not os.path.exists(os.path.join(self.dir, "meta.json")):
            raise SystemExit(f"no run {name!r} (rd ls; rd new {name} --from ..)")
        self.lock = None
        if lock:
            self.lock = open(os.path.join(self.dir, ".lock"), "w")
            try:
                fcntl.flock(self.lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError:
                self.lock.close()
                raise SystemExit(f"run {name} is busy: another rd command is working on it — wait for it")
        self.meta_p, self.sess_p, self.movie_p = (os.path.join(self.dir, f) for f in ("meta.json", "session.json", "movie.jsonl"))
        self.meta = json.load(open(self.meta_p)) if os.path.exists(self.meta_p) else {"checkpoints": {}, "poked": False}
        self.movie = Movie.load(self.movie_p) if os.path.exists(self.movie_p) else Movie()
        self.vars = Vars(self.dir)
        self.live, self.sess = None, None

    # ---- persistence
    def save_meta(self):
        self.meta["frame"] = self.movie.frame
        json.dump(self.meta, open(self.meta_p + ".tmp", "w"), indent=1)
        os.replace(self.meta_p + ".tmp", self.meta_p)

    def save_sess(self):
        json.dump(self.sess, open(self.sess_p + ".tmp", "w"))
        os.replace(self.sess_p + ".tmp", self.sess_p)

    def sync_movie(self):
        """the server's movie is the truth after any step / restore"""
        r = self.live.cmd("movie")
        sa, ed = self.movie.standalone, self.movie.edition
        self.movie = Movie.from_server(r, self.movie.save, ed)
        self.movie.standalone = sa
        self.movie.write(self.movie_p, comment=self.name, rel_to=self.dir)
        if "poked=1" in r:
            self.meta["poked"] = True
        self.save_meta()

    # ---- the session
    def attach(self, cached_only=False):
        """a live session at the movie's head: the cached one if it is still there and in step, else a fresh replay
        (cached_only: None instead of the replay)"""
        if self.live:
            return self.live
        s = json.load(open(self.sess_p)) if os.path.exists(self.sess_p) else None
        if s and sess_alive(s) and s.get("skip", "") == SKIP_FN and \
                s.get("bin") == bin_sha():
            try:
                lv = Live(s["port"])
                if int(Live.kv(lv.cmd("ping"))["frame"]) == self.movie.frame:
                    self.live, self.sess = lv, s
                    return lv
                lv.close()
            except (OSError, SessionEnded):
                pass
        if cached_only:
            return None
        if s:
            kill_session(s)
            if s.get("snaps") or s.get("mark") is not None:
                print("[the session was rebuilt (it ended, or rhost was rebuilt): its savestates are gone — checkpoints load by replay, "
                      "`ram mark` has to be taken again]")
        t0 = time.time()
        h = spawn_live(self.name, self.movie)
        try:
            self.live = Live(h["port"])
            k = Live.kv(self.live.cmd("ping"))
        except BaseException:
            kill_session(h)
            raise
        if int(k["frame"]) != self.movie.frame:
            raise SystemExit(f"the replayed session is at frame {k['frame']}, the movie at {self.movie.frame} (see {h['rundir']}/rhost.log on {h['host']})")
        self.sess = dict(h, epoch=int(k["epoch"]), snaps={}, mark=None, skip=SKIP_FN, mask=SKIP_MASK,
                         bin=bin_sha())
        self.save_sess()
        if self.movie.frame > 2000:
            print(f"[session rebuilt by replaying {self.movie.frame} frames in {time.time() - t0:.1f}s]")
        return self.live

    def stop(self):
        s = json.load(open(self.sess_p)) if os.path.exists(self.sess_p) else None
        if s:
            kill_session(s)
            for w in s.get("workers", []):
                kill_session(w)
            os.remove(self.sess_p)

    def epoch_check(self, r):
        """an anchor on the server (explicit, or a search's) invalidated every savestate id we hold"""
        e = Live.kv(r).get("epoch")
        if e and int(e) != self.sess["epoch"]:
            if self.sess.get("mark") is not None:
                print("[the search re-anchored the savestates: the `ram mark` is gone — mark again before the action you want to diff]")
            self.sess.update(epoch=int(e), snaps={}, mark=None)    # undo entries name their (pid, epoch): stale ones replay

    def snap(self):
        r = self.live.cmd("snap")
        self.epoch_check(r)
        return int(Live.kv(r)["id"])

    def sid_ok(self, u):
        """is the undo entry's savestate one of THIS session's (same process, no re-anchor since)?"""
        return bool(self.sess and u.get("sid") is not None and u.get("pid") == self.sess["pid"] and u.get("epoch") == self.sess["epoch"])

    def push_undo(self, tail=False):
        """before every state-changing command: a savestate (instant undo while the session lives) + what a replay
        needs when it does not — the frame, the checkpoints, and for a step back in time (ck load, rewind: tail=True)
        a copy of the movie it cuts. The stack lives in meta.json: it survives session rebuilds."""
        u = {"sid": self.snap() if self.live else None, "pid": self.sess and self.sess["pid"], "epoch": self.sess and self.sess["epoch"],
             "frame": self.movie.frame, "cks": dict(self.meta.get("checkpoints", {})), "tail": None}
        if tail:
            os.makedirs(os.path.join(self.dir, "undo"), exist_ok=True)
            u["tail"] = f"undo/{int(time.time() * 1000)}-f{self.movie.frame}.jsonl"
            self.movie.write(os.path.join(self.dir, u["tail"]), comment=self.name + " (undo copy)")
        stack = self.meta.setdefault("undo", [])
        stack.append(u)
        while len(stack) > 8:
            old = stack.pop(0)
            if self.sid_ok(old) and old["sid"] not in ({v[0] for v in self.sess["snaps"].values()} | {self.sess.get("mark")}):
                self.live.cmd(f"free {old['sid']}", check=False)
            if old.get("tail") and os.path.exists(os.path.join(self.dir, old["tail"])):
                os.remove(os.path.join(self.dir, old["tail"]))
        self.save_meta()

    def drop_undo(self):
        """take back the entry a command pushed when it ended up changing nothing (a search that applied no path): `rd undo`
        must step back over the last change, not over a no-op"""
        stack = self.meta.get("undo", [])
        if not stack:
            return
        u = stack.pop()
        if self.live and self.sid_ok(u) and u["sid"] not in ({v[0] for v in self.sess["snaps"].values()} | {self.sess.get("mark")}):
            self.live.cmd(f"free {u['sid']}", check=False)
        if u.get("tail") and os.path.exists(os.path.join(self.dir, u["tail"])):
            os.remove(os.path.join(self.dir, u["tail"]))
        self.save_meta()

    # ---- reading state
    def ev(self, *exprs):
        return eval_exprs(self.attach(), self.vars, exprs)

    def status_line(self):
        lv = self.attach()
        k = Live.kv(lv.cmd("ping"))
        names = [n for n, d in self.vars.defs.items() if d.get("status")]
        vals = self.ev(*names) if names else []
        s = f"frame {k['frame']}  rng {k['rng']}  " + "  ".join(f"{n}={fmt(v)}" for n, v in zip(names, vals))
        if self.meta.get("poked"):
            s += "  [POKED: memory was written by hand — this run's movie does not reproduce it]"
        return s

    # ---- acting
    def play(self, spec, until=None, avoid=None, watch=(), every=0, quiet=False, rows=40, undo=True):
        lv = self.attach()
        segs, escs, total = spec_to_run(parse_spec(spec), lambda e: lv.prog(self.vars.compile(e)))
        if not total:
            raise SystemExit("empty input spec")
        if undo:
            self.push_undo()
        cmd = f"run {segs}"
        if escs:
            cmd += " esc=" + ",".join(map(str, escs))
        if until:
            cmd += f" u={lv.prog(self.vars.compile(until))}"
        if avoid:
            cmd += f" a={lv.prog(self.vars.compile(avoid))}"
        ids, wsl = sampled_cols(lv, self.vars, watch)
        ev_names = [n for n, d in self.vars.defs.items() if d.get("event")]
        if len(ids) + len(ev_names) > MAX_SAMPLED:      # the watch columns first; the event names get what is left
            if not quiet:
                print(f"[--watch takes {len(ids)} of the {MAX_SAMPLED} sampled programs: events not tracked this call: "
                      f"{' '.join(ev_names[MAX_SAMPLED - len(ids):])}]")
            ev_names = ev_names[:MAX_SAMPLED - len(ids)]
        ids += [lv.prog(self.vars.program(e)) for e in ev_names]
        nw = len(ids) - len(ev_names)
        cols = list(watch) + ev_names
        if cols:
            cmd += " s=" + ",".join(map(str, ids))
            cmd += f" e={every if (every and watch) else 1}"
        r = lv.cmd(cmd)
        k = Live.kv(r)
        self.sync_movie()
        self.save_sess()
        out = {"ran": int(k["ran"]), "stop": k["stop"], "frame": int(k["frame"]), "rows": [], "events": []}
        if cols and k.get("samples"):
            prev = None
            for f, vals in parse_samples(k["samples"]):
                if watch and (not every or (f - (out["frame"] - out["ran"])) % every == 0):
                    out["rows"].append((f, fold_row(vals[:nw], wsl)))
                evs = vals[nw:]
                if prev is not None:
                    for n, a, b in zip(ev_names, prev, evs):
                        if a != b:
                            out["events"].append((f, n, a, b))
                prev = evs
        if not quiet:
            print(f"ran {out['ran']} ticks ({out['stop']})  " + self.status_line())
            by = {}
            for f, n, a, b in out["events"]:
                by.setdefault(n, []).append((f, a, b))
            busy = {n for n, v in by.items() if len(v) > 8}        # a counter / a position: one line, not one per tick
            for n in sorted(busy):
                v = by[n]
                vals = [x[1] for x in v] + [v[-1][2]]
                print(f"  {n}: {len(v)} changes f{v[0][0]}..f{v[-1][0]}, {fmt(v[0][1])} → {fmt(v[-1][2])} (range {fmt(min(vals))}..{fmt(max(vals))}); "
                      f"first f{v[0][0]} {fmt(v[0][1])}→{fmt(v[0][2])}, last f{v[-1][0]} {fmt(v[-1][1])}→{fmt(v[-1][2])}")
            sparse = [e for e in out["events"] if e[1] not in busy]
            for f, n, a, b in sparse[:60]:
                print(f"  f{f}: {n} {fmt(a)} → {fmt(b)}")
            if len(sparse) > 60:
                print(f"  … {len(sparse) - 60} more events")
            if watch:
                print("  frame  " + "  ".join(watch))
                if len(out["rows"]) > rows:
                    print(f"  … {len(out['rows']) - rows} earlier rows not shown (--rows N)")
                for f, vals in out["rows"][-rows:]:
                    print(f"  {f:6d} " + "  ".join(fmt(v) for v in vals))
        return out

    def load_frame(self, frame, sid=None, movie=None):
        """go to FRAME of this movie (or of MOVIE: an undo's copy): by savestate when one is held, else by a fresh
        replay of the prefix"""
        if sid is not None and self.live:
            r = self.live.cmd(f"restore {sid}", check=False)
            if r.startswith("ok") and int(Live.kv(r)["frame"]) == frame:
                self.sync_movie()
                return "savestate"
        if self.live:
            self.live.close()
        self.stop()
        self.live = None
        self.movie = (movie or self.movie).truncated(frame)
        self.movie.write(self.movie_p, comment=self.name, rel_to=self.dir)
        self.save_meta()
        self.attach()
        return "replay"

    def close(self):
        if self.live:
            self.live.close()
            self.live = None
        if self.lock:
            self.lock.close()
            self.lock = None


def fmt(v):
    if isinstance(v, float) and v != int(v):
        return f"{v:.4f}".rstrip("0").rstrip(".")
    return str(int(v))


# ------------------------------------------------------------------ earlier frames: rd scan / rd eval --at-frame
def replay_window(lv, movie, at, lo, hi, V, exprs):
    """LV stands at frame AT (≤ LO) of MOVIE: feed it MOVIE's own inputs (masks, ESC posts, rngseed ops) to LO, then
    sample EXPRS after every tick LO..HI → [(frame, [values])]. LO == HI: one row, any number of expressions."""
    k = Live.kv(lv.cmd("ping"))
    if int(k["frame"]) != at:
        raise SystemExit(f"scan: the session is at frame {k['frame']}, expected {at}")
    for a, b in movie.chunks(at, lo):
        k = Live.kv(lv.cmd("run " + movie.run_args(a, b)))
        if int(k["frame"]) != b:
            raise SystemExit(f"scan: the replay stopped at frame {k['frame']} (run {a}..{b}: {k.get('stop')})")
    if lo == hi:
        return [(lo, eval_exprs(lv, V, exprs))]
    ids, sl = sampled_cols(lv, V, exprs)
    first = [float(f"{float(x):.9g}") for x in Live.kv(lv.cmd("eval " + ",".join(map(str, ids))))["v"].split(":")]
    rows = [(lo, fold_row(first, sl))]           # (%.9g: as `run` prints its samples — one value, one spelling)
    for a, b in movie.chunks(lo, hi):
        k = Live.kv(lv.cmd(f"run {movie.run_args(a, b)} s={','.join(map(str, ids))} e=1"))
        got = parse_samples(k.get("samples"))
        if int(k["frame"]) != b or len(got) != b - a:
            raise SystemExit(f"scan: run {a}..{b} ended at frame {k['frame']} with {len(got)} rows")
        rows += [(f, fold_row(v, sl)) for f, v in got]
    return rows


def scan_frames(movie, V, lo, hi, exprs, live_run=None, tag="scan"):
    """EXPRS at every frame LO..HI of MOVIE's replay → (rows, info). Never changes a run. Two ways:
    - LIVE_RUN (a locked Run whose live session plays exactly MOVIE) holds a checkpoint savestate at or before LO: take a
      savestate at its head, restore the nearest checkpoint (after checking the session's movie there is MOVIE's), play
      MOVIE's inputs on, restore the head, free the savestate. The run's movie, checkpoints, undo stack stay as they were;
    - else a FRESH side session (the best host, as `rd new` picks it) replays MOVIE from boot to LO; stopped afterwards."""
    t0, info = time.time(), {}
    if live_run is not None and live_run.attach(cached_only=True):
        r, lv = live_run, live_run.live
        cks = r.meta.get("checkpoints", {})
        cands = [(sf, n, sid) for n, (sid, sf) in (r.sess.get("snaps") or {}).items()
                 if n in cks and cks[n]["frame"] == sf and sf <= lo]
        if r.movie.frame <= lo:
            cands.append((r.movie.frame, "head", None))
        best = max(cands, default=None)
        if best and best[2] is None:                    # the head itself (LO = HI = the run's frame)
            rows = replay_window(lv, movie, best[0], lo, hi, V, exprs)
            return rows, dict(how="the run's live session at its head", host=r.sess.get("host", "wine"), reach=0,
                              wall=time.time() - t0)
        if best:
            sf, n, sid = best
            sr = lv.cmd("snap", check=False)
            if sr.startswith("ok") and int(Live.kv(sr).get("epoch", -1)) == r.sess.get("epoch"):
                here = int(Live.kv(sr)["id"])
                try:
                    rr = lv.cmd(f"restore {sid}", check=False)
                    if rr.startswith("ok") and int(Live.kv(rr)["frame"]) == sf and \
                            Movie.from_server(lv.cmd("movie"), None).ops == movie.truncated(sf).ops:
                        rows = replay_window(lv, movie, sf, lo, hi, V, exprs)
                        return rows, dict(how=f"the run's live session from checkpoint {n} (f{sf})", host=r.sess.get("host", "wine"),
                                          reach=lo - sf, wall=time.time() - t0)
                    print(f"[checkpoint {n}'s savestate is not this movie's any more: replaying from boot instead]")
                finally:
                    lv.cmd(f"restore {here}")
                    lv.cmd(f"free {here}")
                    k = Live.kv(lv.cmd("ping"))
                    if int(k["frame"]) != r.movie.frame:
                        raise SystemExit(f"scan: the run's session did not return to its head (f{k['frame']}, movie f{r.movie.frame})")
            elif sr.startswith("ok"):                    # (the session.json epoch was stale: keep the truth)
                lv.cmd(f"free {Live.kv(sr)['id']}", check=False)
                r.epoch_check(sr)
                r.save_sess()
    h = spawn_live(tag, movie.truncated(hi), upto=lo, idle=300)
    try:
        lv = Live(h["port"])
        try:
            lv.cmd("ping")                              # answers once the replay to LO is done
            t1 = time.time()
            rows = replay_window(lv, movie, lo, lo, hi, V, exprs)
        finally:
            lv.close()
    finally:
        kill_session(h)
    return rows, dict(how=f"a fresh session replaying from boot", host=h["host"], reach=lo, wall=time.time() - t0,
                      spawn=t1 - t0, sample=time.time() - t1)


# ------------------------------------------------------------------ start points
def scenario_end_save(name):
    """the user\\ dir a pure replay of scenario NAME leaves (rhost.py final_save: cached, re-derivable) → its _saves copy"""
    target = "win" if host_up("win") else "wine"
    d, digest = RH.final_save(name, target)
    if not RH.save_slots(d):
        raise SystemExit(f"end:{name}: that scenario leaves no slot save (save at a statue before its end)")
    return store_user_dir(d)[1]


def cmd_endsave(a):
    """the user\\ dir this run's movie has written (a statue save) → a _saves dir: `rd new R2 --from save:DIGEST` starts from it"""
    r = Run(a.run)
    r.attach()
    sp = os.path.join(r.dir, ".endsave")
    shutil.rmtree(sp, ignore_errors=True)
    if not sess_file(r.sess, "user", sp) or not RH.save_slots(sp):
        raise SystemExit("this run has no slot save (save at a goddess statue first)")
    digest, path = store_user_dir(sp)
    shutil.rmtree(sp, ignore_errors=True)
    if r.movie.save and RH.save_digest(r.movie.save) == digest:
        raise SystemExit("this run has not written a save of its own (the user\\ dir is its start save)")
    r.meta["endsave"] = {"digest": digest, "frame": r.movie.frame, "slots": RH.save_slots(path)}
    r.save_meta()
    print(f"end save {digest} (frame {r.movie.frame}, slots {' '.join(RH.save_slots(path))}): rd new NEXT --from save:{digest}")
    r.close()


def resolve_start(src):
    """--from → (Movie, replay source trace or None, frame limit)"""
    # a session starts serving after the first Flip: frame 1 is the earliest state there is
    if src == "fresh":
        return Movie(None, [], 1), None, None
    if src.startswith("end:"):
        return Movie(scenario_end_save(src[4:]), [], 1), None, None
    if src.startswith("save:"):
        return Movie(save_path(src[5:]), [], 1), None, None
    m = re.fullmatch(r"([^:@]+)(?::([^@]+))?(?:@(\d+))?", src)
    if not m:
        raise SystemExit(f"--from {src!r}: fresh | SCENARIO[@F] | RUN[:CHECKPOINT|@F] | end:SCENARIO | save:DIGEST")
    name, ck, fr = m.group(1), m.group(2), m.group(3) and int(m.group(3))
    rd = os.path.join(RUNS, name)
    if os.path.exists(os.path.join(rd, "movie.jsonl")):
        mv = Movie.load(os.path.join(rd, "movie.jsonl"))
        if ck:
            cks = json.load(open(os.path.join(rd, "meta.json"))).get("checkpoints", {})
            if ck not in cks:
                raise SystemExit(f"run {name} has no checkpoint {ck!r} ({', '.join(cks) or 'none'})")
            fr = cks[ck]["frame"]
        return (mv.truncated(fr) if fr is not None else mv), None, None
    if os.path.exists(os.path.join(RH.scen_dir(name), "trace.jsonl")):
        tr, save, ed = RH.scenario_inputs(name, argparse.Namespace(target="win" if host_up("win") else "wine"))
        return Movie(save, [], 0, ed or "enold"), tr, fr
    raise SystemExit(f"--from {src!r}: no such run or scenario")


def cmd_new(a):
    rd = os.path.join(RUNS, a.run)
    if os.path.exists(os.path.join(rd, "meta.json")):
        if not a.force:
            raise SystemExit(f"run {a.run} exists (--force replaces it)")
        r = Run(a.run); r.stop(); r.close()
        shutil.rmtree(rd)
    mv, trace, limit = resolve_start(a.src)
    mv.standalone = bool(getattr(a, "standalone", False))
    if mv.standalone:
        print("[standalone: the port alone, no retail code — every session of this run]")
    r = Run(a.run, create=True)
    r.meta.update(created=time.strftime("%Y-%m-%d %H:%M:%S"), start=a.src)
    r.movie = mv
    if trace:
        # an existing scenario (any trace dialect): replay it in a live session; its resolved inputs become our movie
        t0 = time.time()
        h = spawn_live(a.run + "-new", mv, src_trace=trace, limit=limit, standalone=mv.standalone)
        try:
            r.live = Live(h["port"])
            k = Live.kv(r.live.cmd("ping"))
        except BaseException:
            kill_session(h)
            raise
        r.sess = dict(h, epoch=int(k["epoch"]), snaps={}, skip=SKIP_FN, mask=SKIP_MASK, mark=None,
                      bin=bin_sha())
        r.sync_movie()
        r.save_sess()
        print(f"[replayed {a.src} to frame {r.movie.frame} in {time.time() - t0:.1f}s]")
    else:
        r.movie.write(r.movie_p, comment=a.run, rel_to=r.dir)
        r.save_meta()
        r.attach()
    print(f"run {a.run}: {r.status_line()}")
    r.close()


def cmd_play(a):
    r = Run(a.run)
    r.play(a.spec, until=a.until, avoid=a.avoid, watch=split_top(a.watch), every=a.every, rows=a.rows)
    r.close()


def cmd_tap(a):
    r = Run(a.run)
    if not a.text:
        r.play(f"({a.spec} .*{a.gap})x{a.n}", until=a.until, avoid=a.avoid)
        r.close()
        return
    # a transcript: the text on screen just before each press, rows new since the previous press only
    lv = r.attach()
    r.push_undo()
    f0, seen, why, held = r.movie.frame, set(), "spec ended", None

    def flush(now=()):
        """print the held sample, minus the rows that were still being typed (a prefix of the row now at that spot)"""
        nonlocal held
        if held:
            at = {(x, y): t for x, y, t in now}
            keep = [(x, y, t) for x, y, t in held[1] if not (at.get((x, y), "") != t and at.get((x, y), "").startswith(t))]
            if keep:
                print(f"f{held[0]}:")
                for x, y, t in keep:
                    print(f"  {x:5d} {y:4d}  {t}")
        held = None

    def show():
        nonlocal held
        rows = [(x, y, t if bud < 0 else t[:bud]) for x, y, col, bud, t in screen_text(lv) if t.strip()]
        flush(rows)
        new = [w for w in rows if w not in seen]
        if new:
            held = (r.movie.frame, new)
        seen.clear(); seen.update(rows)
    for _ in range(a.n):
        show()
        o = r.play(f"{a.spec} .*{a.gap}", until=a.until, avoid=a.avoid, quiet=True, undo=False)
        if o["stop"] != "end":
            why = o["stop"]
            break
    show()
    flush()
    print(f"ran {r.movie.frame - f0} ticks ({why})  " + r.status_line())
    r.close()


def screen_text(lv):
    """the frame's TextOutA calls (one per glyph or word) merged into lines: [(y, x, text, color)]"""
    r = lv.cmd("text")
    rows = Live.kv(r).get("rows", "")
    glyphs = []
    for item in filter(None, rows.split(";")):
        x, y, col, hx = item.split(":", 3)
        glyphs.append((int(y), int(x), bytes.fromhex(hx).decode("cp932", errors="replace"), col))
    # outlined / shadowed text draws each glyph several times, ±1..2 px apart in other colours: keep the LAST draw
    # (the face drawn over its outline) of each glyph at (about) one place
    kept = []
    for g in reversed(glyphs):
        if not any(k[2] == g[2] and abs(k[0] - g[0]) <= 2 and abs(k[1] - g[1]) <= 2 for k in kept[-24:]):
            kept.append(g)
    glyphs = kept[::-1]
    lines = []
    for y, x, t, col in sorted(glyphs, key=lambda g: (g[0], g[1])):
        if lines and lines[-1][0] == y and -2 <= x - lines[-1][4] <= 16:
            ly, lx, lt, lc, _ = lines[-1]
            gap = " " if x - lines[-1][4] > 9 else ""
            lines[-1] = (ly, lx, lt + gap + t, lc, x + 7 * len(t))
        else:
            lines.append((y, x, t, col, x + 7 * len(t)))
    return [(y, x, t, c) for y, x, t, c, _ in lines]


def cmd_text(a):
    r = Run(a.run)
    for y, x, t, c in screen_text(r.attach()):
        print(f"  y{y:4d} x{x:4d}  {t}")
    r.close()


def cmd_status(a):
    r = Run(a.run)
    print(r.status_line())
    cks = r.meta.get("checkpoints", {})
    if cks:
        print("checkpoints: " + "  ".join(f"{n}@{c['frame']}" for n, c in sorted(cks.items(), key=lambda kv: kv[1]["frame"])))
    r.close()


def cmd_eval(a):
    r = Run(a.run)
    if a.at or a.at_frame is not None:
        # peek at an earlier frame: the nearest checkpoint savestate of the live session (restored, then the head again),
        # else a fresh side session replaying the movie — the run itself does not move
        cks = r.meta.get("checkpoints", {})
        if a.at and a.at not in cks:
            raise SystemExit(f"no checkpoint {a.at!r} ({', '.join(cks) or 'none'})")
        fr = cks[a.at]["frame"] if a.at else a.at_frame
        if not 0 <= fr <= r.movie.frame:
            raise SystemExit(f"frame {fr} is outside the movie (0..{r.movie.frame})")
        rows, info = scan_frames(r.movie, r.vars, fr, fr, a.exprs, live_run=r, tag=f"{r.name}-at")
        vals = rows[0][1]
        print(f"at {'checkpoint ' + a.at + ', ' if a.at else ''}frame {fr} ({info['how']}, {info['host']}, {info['wall']:.1f}s); "
              f"the run stays at frame {r.movie.frame}")
    else:
        vals = r.ev(*a.exprs)
    for e, v in zip(a.exprs, vals):
        print(f"{e} = {repr(v) if a.raw else fmt(v)}" + (f"  (0x{int(v) & 0xffffffff:x})" if isinstance(v, int) and abs(v) > 9 else ""))
    r.close()


def scan_filter(rows, lo, nw, every=0, changes=False, where=False):
    """rows [(f, [watch values.., where value])] → the rows to print: every N-th tick from LO, only those whose watched
    values differ from the previous printed row (--changes), only where the last column holds (--where)"""
    out, prev = [], None
    for f, v in rows:
        if every and (f - lo) % every:
            continue
        if where and not v[nw]:
            continue
        if changes and prev is not None and v[:nw] == prev:
            continue
        out.append((f, v[:nw]))
        prev = v[:nw]
    return out


def cmd_scan(a):
    exprs = split_top(a.watch)
    if not exprs:
        raise SystemExit("--watch EXPR,EXPR..: what to read at each frame")
    view = Run(a.run, lock=False)                      # the names (and, without --at-run, the movie): read only
    src = Run(a.at_run, lock=False) if a.at_run else view
    V = Vars(view.dir, also=[src.dir] if a.at_run else ())
    live_run = None
    if not a.fresh and not a.at_run and os.path.exists(view.sess_p):
        try:                                           # its live session's checkpoints are a head start
            live_run = Run(a.run)
        except SystemExit:
            live_run = None
    try:
        movie = (live_run or src).movie
        lo, hi = a.frm, movie.frame if a.to is None else a.to
        if not 0 <= lo <= hi <= movie.frame:
            raise SystemExit(f"--from {lo} --to {hi}: the movie of {src.name} is 0..{movie.frame}")
        cols = exprs + ([a.where] if a.where else [])
        rows, info = scan_frames(movie, V, lo, hi, cols, live_run=live_run, tag=f"{a.run}-scan")
    finally:
        if live_run:
            live_run.close()
    shown = scan_filter(rows, lo, len(exprs), a.every, a.changes, bool(a.where))
    sampled = hi - lo + 1
    print(f"scan {src.name} f{lo}..f{hi}: {sampled} frames in {info['wall']:.1f}s — {info['how']} on {info['host']}, "
          f"{info['reach']} ticks to f{lo}" + (f" (spawn + replay {info['spawn']:.1f}s, sampling {info['sample']:.2f}s = "
                                              f"{sampled / max(info['sample'], 1e-3):.0f} frames/s)" if "spawn" in info else "")
          + (f"; {len(shown)} rows shown" if len(shown) != len(rows) else ""))
    print("  frame  " + "  ".join(exprs))
    show = (lambda x: repr(x)) if a.raw else fmt
    for f, v in shown:
        print(f"  {f:6d} " + "  ".join(show(x) for x in v))


def cmd_ram(a):
    r = Run(a.run)
    if a.op == "list":
        for n, d in sorted(r.vars.defs.items()):
            what = d.get("expr") or f"{d.get('type', 's32')} @ {d['addr']}"
            flags = "".join(f" [{f}]" for f in ("status", "event") if d.get(f))
            print(f"{n:16s} {what:44s}{flags}  {d.get('note', '')}")
    elif a.op == "status":
        print(r.status_line())
    elif a.op == "read":
        va, n = int(a.args[0], 16), int(a.args[1], 0) if len(a.args) > 1 else 64
        h = bytes.fromhex(r.attach().cmd(f"r {va:x} {n}").split()[1])
        for i in range(0, len(h), 16):
            row = h[i:i + 16]
            dw = " ".join(f"{int.from_bytes(row[j:j + 4], 'little'):08x}" for j in range(0, len(row) - 3, 4))
            print(f"{va + i:08x}  {dw}  {''.join(chr(b) if 32 <= b < 127 else '.' for b in row)}")
    elif a.op == "actors":                     # the dungeon actor table: every slot in use
        import struct
        lv = r.attach()
        px, pz = r.ev("px", "pz")
        print("  i type use     hp       x       y       z    yaw  st    t  sub  ae4   pow  dist  unhit stag   age   (aI<field> names: `rd ram R list`)")
        for i in range(128):
            h = bytes.fromhex(lv.cmd(f"r {0x76b970 + i * 0xba4:x} {0xba4}").split()[1])
            s32_ = lambda o: int.from_bytes(h[o:o + 4], "little", signed=True)
            f32_ = lambda o: struct.unpack("<f", h[o:o + 4])[0]
            if not s32_(0x428):
                continue
            x, z = f32_(0x3f0), f32_(0x3f8)
            print(f"{i:3d} {s32_(0x424):4d} {s32_(0x428):3d} {f32_(0x42c):6.0f} {x:7.2f} {f32_(0x3f4):7.2f} {z:7.2f} {f32_(0x420):6.2f} {s32_(0xa54):3d} {s32_(0xa58):4d} {s32_(0xa5c):4d} "
                  f"{s32_(0xae4):4d} {f32_(0xab8):5.2f} {math.hypot(x - px, z - pz):5.1f} {s32_(0xb08):6d} {s32_(0x440):4d} {s32_(0x398):5d}")
    elif a.op == "dwords":                     # N dwords from VA: hex, signed, float
        import struct
        va, n = int(a.args[0], 16), int(a.args[1], 0) if len(a.args) > 1 else 16
        h = bytes.fromhex(r.attach().cmd(f"r {va:x} {n * 4}").split()[1])
        for i in range(n):
            u = int.from_bytes(h[i * 4:i * 4 + 4], "little")
            f = struct.unpack("<f", h[i * 4:i * 4 + 4])[0]
            print(f"{va + i * 4:08x}  [{i:3d}]  {u:08x}  {s32(u):>11d}" + (f"  {f:.6g}" if u and 1e-6 < abs(f) < 1e9 else ""))
    elif a.op == "str":                        # the C string at VA (cp932), up to N bytes
        va, n = int(a.args[0], 16), int(a.args[1], 0) if len(a.args) > 1 else 128
        h = bytes.fromhex(r.attach().cmd(f"r {va:x} {n}").split()[1])
        print(repr(h.split(b"\0")[0].decode("cp932", errors="replace")))
    elif a.op == "define":
        if not a.args:
            raise SystemExit("rd ram R define NAME --addr HEX [--type u8|s8|u16|s16|u32|s32|f32|f64] | --expr EXPR [--status] [--event] [--note ..]")
        d = {}
        if a.expr:
            d["expr"] = a.expr
        elif a.addr:
            d.update(addr=f"0x{int(a.addr, 16):x}", type=a.type)
            if a.type not in TYPES:
                raise SystemExit(f"--type: one of {' '.join(TYPES)}")
        else:
            raise SystemExit("define needs --addr or --expr")
        for f in ("status", "event"):
            if getattr(a, f):
                d[f] = True
        if a.note:
            d["note"] = a.note
        r.vars.define(a.args[0], d, shared=a.shared)
        v = r.ev(a.args[0])[0]
        print(f"{a.args[0]} = {fmt(v)}  ({'tools/drive/vars.json' if a.shared else 'this run'})")
    elif a.op == "mark":
        lv = r.attach()
        r.sess["mark"] = r.snap()
        r.save_sess()
        print(f"marked frame {r.movie.frame}: now act (rd play ..), then `rd ram {a.run} diff`")
    elif a.op == "diff":
        lv = r.attach()
        if not r.sess.get("mark"):
            raise SystemExit("no mark (rd ram R mark) — or a search / anchor since dropped it")
        cmd = f"diff {r.sess['mark']} max={a.max}"
        if a.lo:
            cmd += f" lo={int(a.lo, 16):x}"
        if a.hi:
            cmd += f" hi={int(a.hi, 16):x}"
        k = Live.kv(lv.cmd(cmd))
        rows = [tuple(int(x, 16) for x in t.split(":")) for t in filter(None, k.get("d", "").split(","))]
        minus = set()
        if a.minus:
            minus = {int(l.split()[0], 16) for l in open(os.path.join(r.dir, a.minus + ".diff")) if l.strip()}
        rows = [x for x in rows if x[0] not in minus]
        if a.save:
            with open(os.path.join(r.dir, a.save + ".diff"), "w") as f:
                for va, was, now in rows:
                    f.write(f"{va:08x} {was:08x} {now:08x}\n")
        import struct
        print(f"{k['n']} dwords differ from the mark" + (f" ({len(rows)} after --minus {a.minus})" if a.minus else "") +
              (f" → saved as {a.save}" if a.save else ""))
        for va, was, now in rows[:a.show]:
            fw, fn = struct.unpack("<f", struct.pack("<I", was))[0], struct.unpack("<f", struct.pack("<I", now))[0]
            fl = f"  f32 {fw:.4g} → {fn:.4g}" if all(1e-6 < abs(x) < 1e7 or x == 0 for x in (fw, fn)) else ""
            print(f"  {va:08x}  {was:08x} → {now:08x}  ({s32(was)} → {s32(now)}){fl}")
        if len(rows) > a.show:
            print(f"  … {len(rows) - a.show} more (--show N, --lo/--hi VA to narrow, --minus NAME to drop a noise set)")
    else:
        raise SystemExit("rd ram R list | status | read VA [NBYTES] | dwords VA [N] | str VA [N] | define NAME .. | mark | diff")
    r.close()


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def cmd_ck(a):
    r = Run(a.run)
    cks = r.meta.setdefault("checkpoints", {})
    if a.op == "list":
        for n, c in sorted(cks.items(), key=lambda kv: kv[1]["frame"]):
            print(f"{n:20s} frame {c['frame']:>7d}  {c.get('note', '')}")
    elif a.op == "save":
        r.attach()
        sid = r.snap()
        cks[a.name] = {"frame": r.movie.frame, "note": " ".join(a.note)}
        r.sess["snaps"][a.name] = [sid, r.movie.frame]
        r.save_meta(); r.save_sess()
        print(f"checkpoint {a.name} at frame {r.movie.frame}")
    elif a.op == "load":
        if a.name not in cks:
            raise SystemExit(f"no checkpoint {a.name!r} ({', '.join(cks) or 'none'})")
        fr = cks[a.name]["frame"]
        was = r.movie.frame
        r.attach(cached_only=True)                     # no session: straight to the prefix replay
        r.push_undo(tail=True)
        how = r.load_frame(fr, (r.sess["snaps"].get(a.name) or [None])[0] if r.sess else None)
        dropped = [n for n, c in cks.items() if c["frame"] > fr]
        for n in dropped:
            del cks[n]                                 # checkpoints of the abandoned future
        r.save_meta(); r.save_sess()
        print(f"loaded {a.name} ({how})  " + r.status_line())
        if was > fr:
            print(f"  the movie now ends at f{fr} (was f{was})" + (f"; checkpoints dropped: {' '.join(dropped)}" if dropped else "") +
                  f" — `rd undo {a.run}` brings it all back. To only LOOK at an earlier state: rd eval {a.run} EXPR --at {a.name}")
    r.close()


def cmd_undo(a):
    r = Run(a.run)
    stack = r.meta.get("undo", [])
    if not stack:
        raise SystemExit("nothing to undo (rd rewind R --to F replays a prefix)")
    u = stack.pop()
    tail = os.path.join(r.dir, u["tail"]) if u.get("tail") else None
    movie = Movie.load(tail) if tail and os.path.exists(tail) else None
    if u["frame"] > r.movie.frame and not movie:
        raise SystemExit(f"the undo copy of the movie is gone ({u.get('tail')}): cannot return to f{u['frame']}")
    r.attach(cached_only=True)
    how = r.load_frame(u["frame"], u["sid"] if r.sid_ok(u) else None, movie=movie)
    r.meta["checkpoints"] = {n: c for n, c in u["cks"].items() if c["frame"] <= u["frame"]}
    if tail and os.path.exists(tail):
        os.remove(tail)
    r.save_meta(); r.save_sess()
    print(f"undone ({how})  " + r.status_line())
    r.close()


def cmd_rewind(a):
    r = Run(a.run)
    fr = a.to if a.to is not None else r.movie.frame - a.frames
    if fr < 0 or fr > r.movie.frame:
        raise SystemExit(f"frame {fr} is outside the movie (0..{r.movie.frame})")
    r.attach(cached_only=True)
    r.push_undo(tail=True)                             # `rd undo` brings the tail back (by savestate, or by replay)
    # a live checkpoint exactly there = this movie's own savestate: no replay
    sid = next((r.sess["snaps"][n][0] for n, c in r.meta.get("checkpoints", {}).items()
                if r.sess and c["frame"] == fr and r.sess["snaps"].get(n, [None, -1])[1] == fr), None)
    r.load_frame(fr, sid)
    cks = r.meta.get("checkpoints", {})
    for n in [n for n, c in cks.items() if c["frame"] > fr]:
        del cks[n]
    r.save_meta()
    print("rewound  " + r.status_line())
    r.close()


# ------------------------------------------------------------------ search
DEFAULT_ACTIONS = ["R*8", "L*8", "R*2", "L*2", "RC*8", "LC*8", "C*12", "X .*7", "U .*7", "D*8", ".*8"]   # side-scroller: walk, jump (C), attack (X), door (U), crouch
HOLD_ACTIONS = ["."]                                    # --hold's default tail: stand still (each hold action is cycled to N ticks)


def action_arg(specs, prog=None):
    acts = []
    for s in specs:
        seg, escs, total = spec_to_run(parse_spec(s), prog)
        if escs:
            raise SystemExit(f"action {s!r}: ESC (~) cannot be part of a search action — play it yourself")
        if not total:
            raise SystemExit(f"empty action {s!r}")
        acts.append(seg.replace(",", "+"))
    return "|".join(acts)


def search_progs(V, a):
    """every program a search sends, compiled and checked against the server's limits HERE, before any session moves:
    a bad / over-long expression is its own message (with -j N it used to die inside a worker thread: a traceback)"""
    P = {"goal": V.program(a.goal, f"--goal {a.goal!r}"),
         "cell": [prog_check(t, f"--cell {a.cell!r}") for t in V.compile_tuple(a.cell)],
         "avoid": a.avoid and V.program(a.avoid, f"--avoid {a.avoid!r}"),
         "score": a.score and V.program(a.score, f"--score {a.score!r}")}
    check = lambda e: (V.program(e, f"action mask {{{e}}}"), 0)[1]     # noqa: E731 — {EXPR} masks inside action specs
    action_arg(a.actions or DEFAULT_ACTIONS, check)
    if getattr(a, "hold", 0):
        if a.hold < 0:
            raise SystemExit("--hold N: N ≥ 0 ticks")
        action_arg(a.hold_actions or HOLD_ACTIONS, check)
    return P


def explore_cmd(lv, V, a, seed, apply, P=None):
    P = P or search_progs(V, a)
    g = lv.prog(P["goal"])
    cells = [lv.prog(t) for t in P["cell"]]
    cmd = f"explore g={g} c={','.join(map(str, cells))} acts={action_arg(a.actions or DEFAULT_ACTIONS, lambda e: lv.prog(V.compile(e)))}"
    if P["avoid"]:
        cmd += f" a={lv.prog(P['avoid'])}"
    if P["score"]:
        cmd += f" score={lv.prog(P['score'])} greed={a.greed}"
    if getattr(a, "hold", 0):
        cmd += f" hold={a.hold} hacts={action_arg(a.hold_actions or HOLD_ACTIONS, lambda e: lv.prog(V.compile(e)))}"
        if a.hold_no_goal:
            cmd += " hgoal=0"
    cmd += f" t={int(a.time_limit * 1000)} refine={int(a.refine * 1000)} seed={seed} maxlen={a.max_len} sticky={a.sticky} apply={1 if apply else 0}"
    return cmd


def hold_report(results, a):
    """--hold: how many goal arrivals the hold turned away, over every session"""
    if not getattr(a, "hold", 0):
        return
    rej = sum(int(k.get("rejected", 0)) for k, _ in results)
    held = sum(int(k.get("held", 0)) for k, _ in results)
    print(f"  hold {a.hold}: {rej} goal arrival(s) rejected (an --avoid hit or the goal lost within {a.hold} ticks on every hold action), "
          f"{held} survived")


def cmd_search(a):
    if a.apply_best:
        a.apply = True                                 # --apply-best keeps a found path too, not only the best cell's
    r = Run(a.run)
    if not a.cell:
        a.cell = r.vars.defs.get("_cell", {}).get("expr") or "(room, lx // 1600, ly // 1600)"
    P = search_progs(r.vars, a)                        # before anything moves: a bad expression is just its message
    lv = r.attach()
    if r.ev(a.goal)[0]:
        print("the goal already holds here — nothing to search")
        r.close()
        return
    r.push_undo()
    st = {"applied": False}
    try:
        return search_body(r, lv, a, P, st)
    finally:
        if not st["applied"]:                          # nothing played: the undo entry would be a no-op step
            try:
                r.drop_undo()
            except (OSError, SessionEnded):
                pass
        r.save_sess()
        r.close()


def search_body(r, lv, a, P, st):
    t0 = time.time()
    results = []
    if a.jobs <= 1:
        res = lv.cmd(explore_cmd(lv, r.vars, a, a.seed, a.apply, P), check=False)
        if not res.startswith("ok"):
            raise SystemExit("search: " + res)
        r.epoch_check(res)
        results.append((Live.kv(res), "main"))
        if Live.kv(res).get("applied") == "1":
            st["applied"] = True
    else:
        # extra live sessions at the same frame (each replays the movie), one exploration per session with its own seed
        workers, errs = [], []
        main = r.sess.get("host", "wine")
        # the main session's host first; under RD_HOST=auto a worker it does not admit goes to the next host of the order
        # (each worker replays the movie: its savestates are its own anyway)
        order = [main] + ([h for h in HOSTS if h != main] if RD_HOST == "auto" else [])

        passed = {}                         # host → why it did not take a worker (the first reason)

        def boot(i):
            last = None
            for h in order:
                if h != main and not host_up(h):
                    continue
                try:
                    hs = spawn_live(f"{r.name}-w{i}", r.movie, idle=600, wait=0, host=h)   # optional: no slot → fewer workers
                    workers.append((i, hs))
                    return
                except BaseException as e:      # noqa: B902 — SystemExit / NotAdmitted from spawn_live
                    last = e
                    passed.setdefault(h, str(e).split(" — ", 1)[-1][:160])
            errs.append(str(last))
        ts = [threading.Thread(target=boot, args=(i,)) for i in range(1, a.jobs)]
        [t.start() for t in ts]

        def one(lvx, seed, apply, tag):
            try:
                res = lvx.cmd(explore_cmd(lvx, r.vars, a, seed, apply, P), check=False)
            except BaseException as e:          # noqa: B902 — a thread's SystemExit / socket error: keep the message
                res = f"err {type(e).__name__}: {e}"
            results.append((Live.kv(res) if res.startswith("ok") else {"found": "0", "err": res}, tag))
        try:
            [t.join() for t in ts]
            if errs:
                print(f"[{len(errs)} worker(s) did not start: {errs[0][:120]}]")
            workers.sort()
            by = {}
            for tag, h in [("main", main)] + [(f"w{i}", hs["host"]) for i, hs in workers]:
                by.setdefault(h, []).append(tag)
            print(f"[{len(workers) + 1} sessions at frame {r.movie.frame}, {time.time() - t0:.1f}s to start the extra ones; "
                  + ", ".join(f"{h}: {' '.join(t)}" for h, t in by.items()) + "]")
            for h, why in passed.items():
                print(f"  [{h} took no more: {why}]")
            ths = [threading.Thread(target=one, args=(lv, a.seed, False, f"main@{main}"))]
            lvs = []
            for i, h in workers:
                lx = Live(h["port"]); lvs.append(lx)
                ths.append(threading.Thread(target=one, args=(lx, a.seed + i * 7919, False, f"w{i}@{h['host']}")))
            [t.start() for t in ths]
            [t.join() for t in ths]
            [lx.close() for lx in lvs]
        finally:
            ks = [threading.Thread(target=kill_session, args=(h,)) for i, h in workers]
            [t.start() for t in ks]
            [t.join() for t in ks]
        r.sess.update(snaps={}, mark=None)            # the main session's explore re-anchored
        r.sess["epoch"] = int(Live.kv(lv.cmd("ping"))["epoch"])
    errs = [(k["err"], tag) for k, tag in results if "err" in k]
    if errs and len(errs) == len(results):
        raise SystemExit(f"search: {errs[0][0]}  [{errs[0][1]}]")
    for e, tag in errs:
        print(f"[session {tag} failed: {e[:300]}]")
    results = [(k, tag) for k, tag in results if "err" not in k]
    found = [(int(k["frames"]), k, tag) for k, tag in results if k.get("found") == "1"]
    ticks = sum(int(k.get("ticks", 0)) for k, _ in results)
    if len(results) > 1:
        per = {}
        for k, tag in results:
            h = tag.split("@")[-1]
            per.setdefault(h, [0, 0])
            per[h][0] += 1
            per[h][1] += int(k.get("ticks", 0))
        # explore's clock starts after a worker's replay: ticks / --time-limit = the searching rate
        print("  per host: " + ", ".join(f"{h} {n} session(s) {t} ticks = {t / a.time_limit:.0f}/s ({t / a.time_limit / n:.0f} each)"
                                         for h, (n, t) in per.items()))
    cells = max(int(k.get("cells", 0)) for k, _ in results)
    wall = time.time() - t0
    if not found:
        k = results[0][0]
        print(f"found: false   {cells} cells, {ticks} ticks searched in {wall:.1f}s ({ticks / max(wall, 0.01):.0f} ticks/s)")
        print(f"  newest cells (frames@{a.cell}): " + "  ".join(k.get("newest", "").split(";")))
        print("  one cell only = your actions change nothing the cell sees; split the goal at where it got to, add the missing action, "
              "or put the mechanic's state in --cell")
        hold_report(results, a)
        if a.score:                                    # progress to keep: the best-scored cell any session reached
            cands = [(float(k["bscore"]), -int(k["bframes"]), i) for i, (k, _) in enumerate(results) if k.get("bpath")]
            if not cands:
                print(f"  score: nothing scored above the start ({results[0][0].get('score0', '?')})")
            else:
                bs, _, i = max(cands)
                k = results[i][0]
                spec = segs_to_spec(k["bpath"])
                print(f"  best score {fmt(bs)} (start {fmt(float(k['score0']))}) after {k['bframes']} frames")
                if a.save:
                    open(a.save, "w").write(spec + "\n")
                if a.apply_best:
                    lv.cmd("run " + k["bpath"])
                    st["applied"] = True
                    r.sync_movie()
                    print(f"  applied the path to that cell (--apply-best): score now {fmt(r.ev(a.score)[0])}  " + r.status_line())
                    print(f"  search again from here; `rd undo {a.run}` drops it")
                    return 1
                print(f"  path to it: {spec if len(spec) < 600 else spec[:600] + ' …'}\n  (--apply-best plays it, so the next search starts there)")
        r.sync_movie()
        return 1
    frames, k, tag = min(found, key=lambda x: x[0])
    spec = segs_to_spec(k["path"])
    print(f"found: true   {frames} frames  ({cells} cells, {ticks} ticks in {wall:.1f}s = {ticks / max(wall, 0.01):.0f} ticks/s"
          + (f", best of {len(found)}/{len(results)} sessions [{tag}]" if len(results) > 1 else "") + ")")
    if k.get("arrive"):
        hs = (a.hold_actions or HOLD_ACTIONS)[int(k.get("tail", 0))]
        print(f"  goal reached at +{k['arrive']}, then held {a.hold} ticks on hold action {hs!r} (the path includes them)")
    hold_report(results, a)
    print(f"  path: {spec if len(spec) < 900 else spec[:900] + ' …'}")
    if a.save:
        open(a.save, "w").write(spec + "\n")
    if a.apply:
        if a.jobs > 1 or k.get("applied") != "1":
            lv.cmd("run " + k["path"])
        st["applied"] = True
        holds, *av = r.ev(a.goal, *([a.avoid] if a.avoid else []))
        r.sync_movie()
        print(f"  applied: goal {'holds' if holds else 'DOES NOT HOLD'}" + (f", avoid {'TRUE' if av[0] else 'clear'}" if av else "")
              + "  " + r.status_line())
    else:
        r.sync_movie()
        print("  not applied (--apply plays it; --save FILE keeps it: rd play R \"$(cat FILE)\")")
    return 0


# ------------------------------------------------------------------ pictures: a real-device replay with shots
def shots(r, frames, target=None, tag="look"):
    return session_shots(r, frames, tag)


def win_up(timeout=4):
    try:
        return subprocess.run(["ssh", "-o", "BatchMode=yes", "-o", f"ConnectTimeout={timeout}", RH.WIN, "echo ok"], capture_output=True,
                              text=True, timeout=timeout + 8).stdout.strip().endswith("ok")
    except (subprocess.TimeoutExpired, OSError):
        return False


def sheet(paths, out, cols=4, scale=0.375, labels=None):
    from PIL import Image, ImageDraw
    ims = [Image.open(p).convert("RGB") for p in paths]
    w, h = int(ims[0].width * scale), int(ims[0].height * scale)
    rows = -(-len(ims) // cols)
    sh = Image.new("RGB", (cols * w, rows * h), (20, 20, 20))
    d = ImageDraw.Draw(sh)
    for i, im in enumerate(ims):
        x, y = (i % cols) * w, (i // cols) * h
        sh.paste(im.resize((w, h), Image.LANCZOS), (x, y))
        if labels:
            d.rectangle([x, y, x + 8 + 7 * len(labels[i]), y + 14], fill=(0, 0, 0))
            d.text((x + 3, y + 1), labels[i], fill=(255, 255, 0))
    sh.save(out)
    return out


def cmd_look(a):
    r = Run(a.run)
    frames = [int(x) for x in a.at.split(",")] if a.at else [r.movie.frame]
    ps = session_shots(r, frames)
    if len(ps) > 1:
        out = os.path.join(r.dir, "view", f"look-{int(time.time())}.png")
        sheet(ps, out, labels=[f"f{f}" for f in sorted(set(frames))])
        print(out)
    else:
        print(ps[0])
    r.close()


def cmd_strip(a):
    r = Run(a.run, lock=False)
    to = a.to if a.to is not None else r.movie.frame
    frames = list(range(a.frm, to + 1, a.every or max(1, (to - a.frm) // 15)))
    ps = session_shots(r, frames, tag="strip")
    out = os.path.join(r.dir, "view", f"strip-{a.frm}-{to}.png")
    sheet(ps, out, labels=[f"f{f}" for f in frames])
    print(out)


# ------------------------------------------------------------------ verify / check / lint
def cmd_verify(a):
    """the movie replayed in a FRESH headless rhost (no savestates, no server) reaches the session's frame, RNG and state;
    --real: and the Windows real device (real DirectDraw / DirectShow) reaches the same frame, RNG and every frame's pixels"""
    r = Run(a.run)
    lv = r.attach()
    k = Live.kv(lv.cmd("hash"))
    live = {"frame": k["frame"], "rng": k["rng"], "semv": k["semv"]}
    host = r.sess.get("host", "wine")
    tr = os.path.join(r.dir, ".verify.jsonl")
    r.movie.write(tr, comment=r.name + " (verify)")
    if host == "win":
        RH.WIN = r.sess.get("ssh", RH.WIN)
    # a session at frame F has completed F flips; `--frames N` stops at flip #N (N + 1 flips): N = F - 1
    stop = max(0, r.movie.frame - 1)
    b = RH.replay_args(frames=stop, target="wine" if host == "wine" else "win", headless=True, fb_hash=True,
                       edition=r.movie.edition, standalone=r.movie.standalone)
    if host == "bunker":
        b.target = "bunker"
    _, bd, res, out = RH.run_batch(b, [("verify", tr, r.movie.save, r.movie.edition)], "rd-verify")
    v = res.get("verify") or {}
    got = {"frame": str(int(v.get("frame", -2)) + 1), "rng": v.get("rng"), "semv": v.get("state_semv")}
    same = all(str(got[x]) == str(live[x]) for x in ("frame", "rng", "semv"))
    print(f"live session:     frame {live['frame']} rng {live['rng']} semv {live['semv']}  [{host}]")
    print(f"headless replay:  frame {got['frame']} rng {got['rng']} semv {got['semv']}  fb {v.get('fb')}  [{host}]")
    if not same and got["frame"] == live["frame"] and got["rng"] == live["rng"]:
        # the session's state is off while the movie replays the same frame + RNG: keep the evidence (both dumps, the
        # differing words), then rebuild the session by a clean replay and compare again — the movie is what is filed
        print("(game state differs, frame + RNG equal: the SESSION drifted — savestate restores across branches, or a "
              "host word a restore misses. Evidence:)")
        try:
            ev = os.path.join(r.dir, f"verify-drift-f{r.movie.frame}")
            os.makedirs(ev, exist_ok=True)
            name = os.path.basename(r.sess["rundir"]) if host == "wine" else None
            remote = (f"{RH.WROOT}\\runs\\drive\\{name}" if host == "wine" else r.sess["rundir"]) + "\\live.rhs"
            lv.cmd(f"dump {remote}")
            sess_file(r.sess, "live.rhs", os.path.join(ev, "live.rhs"))
            b2 = RH.replay_args(frames=stop, target=b.target, headless=True, edition=r.movie.edition,
                                standalone=r.movie.standalone, dump_at=str(stop))
            _, bd2, _, _ = RH.run_batch(b2, [("verify", tr, r.movie.save, r.movie.edition)], "rd-verify-dump", fetch=("rhost.log", "state_*.rhs"))
            rep = glob.glob(os.path.join(bd2, "verify", "state_*.rhs"))
            if rep and os.path.exists(os.path.join(ev, "live.rhs")):
                shutil.copy(rep[0], os.path.join(ev, "replay.rhs"))
                d = subprocess.run([sys.executable, os.path.join(RH.HERE, "rhost.py"), "diff", os.path.join(ev, "replay.rhs"),
                                    os.path.join(ev, "live.rhs"), "--show", "40"], capture_output=True, text=True).stdout
                open(os.path.join(ev, "diff.txt"), "w").write(d)
                print("\n".join("  " + l for l in d.splitlines()[-44:]))
                print(f"  (kept in {ev}: report it — it names what a restore missed)")
        except (SystemExit, Exception) as e:   # noqa: BLE001 — evidence is best-effort
            print(f"  (could not collect the evidence: {e})")
        r.stop()
        r.live = None
        lv = r.attach()
        k = Live.kv(lv.cmd("hash"))
        same = k["semv"] == got["semv"] and k["frame"] == got["frame"] and k["rng"] == got["rng"]
        print(f"rebuilt session:  frame {k['frame']} rng {k['rng']} semv {k['semv']}  (a clean replay, no savestates)")
    print("MATCH" if same else "MISMATCH")
    if a.real:
        if not win_up():
            raise SystemExit(f"--real needs the Windows box (ssh {RH.WIN})")
        c = RH.replay_args(frames=stop, target="win", fb_hash=True, edition=r.movie.edition)
        _, bd2, res2, out2 = RH.run_batch(c, [("verify", tr, r.movie.save, r.movie.edition)], "rd-verify-real")
        w = res2.get("verify") or {}
        if host != "win":
            h2 = RH.replay_args(frames=stop, target="win", headless=True, fb_hash=True, edition=r.movie.edition)
            _, _, res3, _ = RH.run_batch(h2, [("verify", tr, r.movie.save, r.movie.edition)], "rd-verify-hlwin")
            v = res3.get("verify") or {}
        real = str(int(w.get("frame", -2)) + 1) == live["frame"] and w.get("rng") == live["rng"] and w.get("fb") == v.get("fb")
        print(f"real-device replay: frame {w.get('frame')} rng {w.get('rng')} fb {w.get('fb')}  (Windows, DirectDraw)  "
              f"vs headless (Windows) fb {v.get('fb')}")
        print("REAL MATCH" if real else "REAL MISMATCH")
        same = same and real
    r.close()
    return 0 if same else 1


def load_task(path):
    txt = open(path).read()
    m = re.match(r"---\n(.*?)\n---\n?(.*)", txt, re.S)
    if not m:
        raise SystemExit(f"{path}: needs a --- header ---")
    t = {"name": os.path.splitext(os.path.basename(path))[0], "start": "fresh", "steps": [], "forbid": [], "text": m.group(2).strip()}
    in_list = None
    for line in m.group(1).splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        s = re.match(r"\s+-\s*(.+)$", line)
        if in_list and s:
            item, hit = s.group(1), None
            for c in re.finditer(r":\s+", item):
                try:
                    ast.parse(item[c.end():].strip(), mode="eval")      # Python drops a trailing "# comment" itself
                except SyntaxError:
                    continue
                hit = (item[:c.start()].strip(), item[c.end():].split("#")[0].strip())
                break
            if not hit:
                raise SystemExit(f"{path}: cannot read step {line.strip()!r} (\"- label: EXPRESSION\")")
            t[in_list].append(hit)
            continue
        k = re.match(r"(\w+):\s*(.*?)\s*(?:#.*)?$", line)
        if not k:
            raise SystemExit(f"{path}: cannot read header line {line!r}")
        in_list = k.group(1) if k.group(1) in ("steps", "forbid") else None
        if not in_list:
            t[k.group(1)] = k.group(2)
    if not t["steps"]:
        raise SystemExit(f"{path}: no steps")
    return t


def start_frame(task):
    """steps are judged only after the task's start prefix (a step that already held inside it proves nothing)"""
    mv, trace, limit = resolve_start(task["start"])
    if trace:
        return limit or 0, True
    return mv.frame, False


def walk_movie(movie, V, preds, after=0, stop_when=None):
    """a fresh live session fed the movie tick by tick → for each predicate the frames where it holds (first, count),
    evaluated in order: `preds` = [(name, expr)]; returns per-frame truth via sampling in chunks"""
    h = spawn_live("check", movie, upto=0, idle=300)
    try:
        lv = Live(h["port"])
        ids = [lv.prog(V.compile(e)) for _, e in preds]
        truth = [[] for _ in preds]
        for at, hi in movie.chunks(0, movie.frame):
            k = Live.kv(lv.cmd(f"run {movie.run_args(at, hi)} s={','.join(map(str, ids))} e=1"))
            for row in filter(None, k.get("samples", "").split(";")):
                p = row.split(":")
                f = int(p[0])
                for i, x in enumerate(p[1:]):
                    if float(x) != 0:
                        truth[i].append(f)
        end = Live.kv(lv.cmd("hash"))
        lv.close()
        return truth, end
    finally:
        kill_session(h)


def cmd_check(a):
    r = Run(a.run)
    t = load_task(a.task)
    after, _ = start_frame(t)
    preds = t["steps"] + t["forbid"]
    truth, end = walk_movie(r.movie, r.vars, preds, after)
    ns, at, ok, first_step = len(t["steps"]), after, True, None
    print(f"task {t['name']}: {ns} steps, {len(t['forbid'])} forbids; movie {r.movie.frame} frames; steps judged after frame {after}")
    for i, (name, expr) in enumerate(t["steps"]):
        hit = next((f for f in truth[i] if f > at), None)
        if hit is None:
            ok = False
            print(f"  step {i + 1} NOT REACHED: {name}: {expr}")
            break
        if first_step is None:
            first_step = hit
        print(f"  step {i + 1} at frame {hit}: {name}")
        at = hit
    for j, (name, expr) in enumerate(t["forbid"]):
        bad = [f for f in truth[ns + j] if first_step is not None and f >= first_step]
        if bad:
            ok = False
            print(f"  FORBIDDEN at frame {bad[0]} ({len(bad)} frames): {name}: {expr}")
    print("PASS" if ok else "FAIL")
    r.close()
    return 0 if ok else 1


def cmd_lint(a):
    t = load_task(a.task)
    after, _ = start_frame(t)
    for name in a.on:
        r = Run(name)
        truth, _ = walk_movie(r.movie, r.vars, t["steps"] + t["forbid"])
        print(f"{name} ({r.movie.frame} frames; the task's start prefix ends at {after}):")
        for (label, expr), fs in zip(t["steps"] + t["forbid"], truth):
            if not fs:
                print(f"  never      {label}: {expr}")
            else:
                pre = sum(1 for f in fs if f <= after)
                print(f"  {len(fs):6d} f  first {fs[0]:>7d} last {fs[-1]:>7d}" + (f"  [{pre} inside the start prefix]" if pre else "") + f"  {label}")
        r.close()


# ------------------------------------------------------------------ filing + the port
def file_scenario(r, name, note=""):
    d = os.path.join(RH.SCEN, name)
    os.makedirs(d, exist_ok=True)
    p = os.path.join(d, "trace.jsonl")
    r.movie.write(p, comment=f"{name}: rd run {r.name}{' — ' + note if note else ''}", rel_to=d)
    return p


def cmd_file(a):
    r = Run(a.run)
    if r.meta.get("poked"):
        raise SystemExit("this run was POKED: its movie does not reproduce its state — not filed")
    p = file_scenario(r, a.name, " ".join(a.note))
    print(f"filed {os.path.relpath(p, ROOT)} ({r.movie.frame} frames). Next: rhost.py golden {a.name}; rhost.py triage {a.name}")
    r.close()


def cmd_port(a):
    """the payoff: this movie against the port — file it under a scratch name, bless retail, check hybrid + standalone"""
    r = Run(a.run, lock=False)
    name = a.name or f"_rd-{a.run}"
    file_scenario(r, name, "rd port")
    res = gates(name)
    return 0 if all(ok for _, ok, _ in res) else 1


AGENTS = os.environ.get("RD_AGENTS", "/mnt/ephemeral/opensummoners/drive/agents")


def cmd_accept(a):
    """the acceptance in one go: check (task) → verify --real → file → bless → hybrid → standalone → the driver's report
    into docs/findings/probes/NAME.md"""
    lines = [f"# probe {a.name}", "", f"run `{a.run}`; accepted {time.strftime('%Y-%m-%d %H:%M')}", ""]
    if a.task:
        ns = argparse.Namespace(run=a.run, task=a.task)
        if cmd_check(ns):
            raise SystemExit("check failed: not accepted")
        lines.append(f"check PASS ({a.task})")
    ns = argparse.Namespace(run=a.run, real=not a.no_real)
    if cmd_verify(ns):
        raise SystemExit("verify failed: not accepted")
    lines.append("verify MATCH" + (" (+ real device)" if not a.no_real else ""))
    r = Run(a.run, lock=False)
    p = file_scenario(r, a.name, a.task and os.path.basename(a.task) or "")
    lines.append(f"filed {os.path.relpath(p, ROOT)} ({r.movie.frame} frames)")
    if not a.no_gates:
        for gate, ok, line in gates(a.name):
            lines.append(f"{gate}: {'PASS' if ok else 'FAIL'} — {line}")
    rep = os.path.join(AGENTS, a.run, "report.md")
    if os.path.exists(rep):
        lines += ["", "## driver's report", "", open(rep).read()]
    os.makedirs(os.path.join(ROOT, "docs", "findings", "probes"), exist_ok=True)
    out = os.path.join(ROOT, "docs", "findings", "probes", f"{a.name}.md")
    open(out, "w").write("\n".join(lines) + "\n")
    print(f"report → {os.path.relpath(out, ROOT)}")


def cmd_stop(a):
    r = Run(a.run)
    r.stop()
    print(f"stopped the live session of {a.run} (the run and its movie stay; the next command replays)")
    r.close()


def cmd_ls(a):
    if not os.path.isdir(RUNS):
        return
    for n in sorted(os.listdir(RUNS)):
        mp = os.path.join(RUNS, n, "meta.json")
        if not os.path.exists(mp):
            continue
        m = json.load(open(mp))
        sp = os.path.join(RUNS, n, "session.json")
        s = json.load(open(sp)) if os.path.exists(sp) else None
        live = sess_alive(s)
        print(f"{n:28s} frame {m.get('frame', 0):>7d}  from {m.get('start', '?'):28s} {(s.get('host', 'wine') if live else '-'):6s} "
              f"{' '.join(sorted(m.get('checkpoints', {})))}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sp = ap.add_subparsers(dest="cmd", required=True)
    p = sp.add_parser("new"); p.add_argument("run"); p.add_argument("--from", dest="src", required=True); p.add_argument("--force", action="store_true"); p.add_argument("--standalone", action="store_true", help="the port alone (no retail code) in every session of this run"); p.set_defaults(fn=cmd_new)
    p = sp.add_parser("play"); p.add_argument("run"); p.add_argument("spec"); p.add_argument("--until"); p.add_argument("--avoid")
    p.add_argument("--watch", help="EXPR,EXPR..: print these per sampled tick"); p.add_argument("--every", type=int, default=0)
    p.add_argument("--rows", type=int, default=40, help="watch rows shown (the last N)"); p.set_defaults(fn=cmd_play)
    p = sp.add_parser("tap"); p.add_argument("run"); p.add_argument("spec"); p.add_argument("--n", type=int, default=40); p.add_argument("--gap", type=int, default=8)
    p.add_argument("--until"); p.add_argument("--avoid")
    p.add_argument("--text", action="store_true", help="a transcript: print the text on screen before each press (rows new since the last one)")
    p.set_defaults(fn=cmd_tap)
    p = sp.add_parser("status"); p.add_argument("run"); p.set_defaults(fn=cmd_status)
    p = sp.add_parser("text"); p.add_argument("run"); p.set_defaults(fn=cmd_text)
    p = sp.add_parser("endsave"); p.add_argument("run"); p.set_defaults(fn=cmd_endsave)
    p = sp.add_parser("eval"); p.add_argument("run"); p.add_argument("exprs", nargs="+")
    p.add_argument("--at", help="evaluate at this checkpoint's state; the run does not move (no savestate: by replay)")
    p.add_argument("--at-frame", type=int, help="evaluate at frame F of the run's movie (a replay; the run does not move)")
    p.add_argument("--raw", action="store_true", help="full precision values (default: 4 decimals)")
    p.set_defaults(fn=cmd_eval)
    p = sp.add_parser("scan", help="rows of state at earlier frames: the movie replayed in a side session")
    p.add_argument("run"); p.add_argument("--from", dest="frm", type=int, required=True); p.add_argument("--to", type=int)
    p.add_argument("--watch", required=True, help="EXPR,EXPR..: the columns (commas inside brackets do not split)")
    p.add_argument("--every", type=int, default=0, help="every N-th frame from --from")
    p.add_argument("--changes", action="store_true", help="only rows where a watched value changed")
    p.add_argument("--where", help="only rows where EXPR holds")
    p.add_argument("--at-run", help="replay this run's movie instead (read only); names: R's, then RUN's")
    p.add_argument("--fresh", action="store_true", help="always a fresh side session (not the run's checkpoint savestates)")
    p.add_argument("--raw", action="store_true", help="full precision values (default: 4 decimals)")
    p.set_defaults(fn=cmd_scan)
    p = sp.add_parser("ram"); p.add_argument("run"); p.add_argument("op"); p.add_argument("args", nargs="*")
    p.add_argument("--addr"); p.add_argument("--type", default="s32"); p.add_argument("--expr"); p.add_argument("--status", action="store_true")
    p.add_argument("--event", action="store_true"); p.add_argument("--note"); p.add_argument("--shared", action="store_true", help="define in tools/drive/vars.json (all runs)")
    p.add_argument("--max", type=int, default=4000); p.add_argument("--show", type=int, default=60); p.add_argument("--lo"); p.add_argument("--hi")
    p.add_argument("--minus", help="drop the addresses of a saved diff (a noise set)"); p.add_argument("--save", help="save this diff's addresses under a name")
    p.set_defaults(fn=cmd_ram)
    p = sp.add_parser("ck"); p.add_argument("run"); p.add_argument("op", choices=("save", "load", "list")); p.add_argument("name", nargs="?"); p.add_argument("note", nargs="*"); p.set_defaults(fn=cmd_ck)
    p = sp.add_parser("undo"); p.add_argument("run"); p.set_defaults(fn=cmd_undo)
    p = sp.add_parser("rewind"); p.add_argument("run"); p.add_argument("--to", type=int); p.add_argument("--frames", type=int, default=0); p.set_defaults(fn=cmd_rewind)
    p = sp.add_parser("search"); p.add_argument("run"); p.add_argument("--goal", required=True); p.add_argument("--avoid"); p.add_argument("--cell")
    p.add_argument("--actions", nargs="+"); p.add_argument("--time-limit", type=float, default=60); p.add_argument("--refine", type=float, default=6)
    p.add_argument("--apply", action="store_true"); p.add_argument("--save"); p.add_argument("--seed", type=int, default=1); p.add_argument("--max-len", type=int, default=8)
    p.add_argument("--sticky", type=int, default=60); p.add_argument("-j", "--jobs", type=int, default=1, help="parallel sessions, one exploration each")
    p.add_argument("--score", help="EXPR, higher = closer: cells are expanded in proportion to it (direction), and the best one is reported")
    p.add_argument("--greed", type=float, default=3.0, help="--score: the best cell is picked e^GREED times as often as the worst (default 3)")
    p.add_argument("--apply-best", action="store_true", help="--apply, and with --score: goal not reached in time → keep the path to the best-scored cell")
    p.add_argument("--hold", type=int, default=0, help="a goal arrival counts only if some hold action then survives N ticks: --avoid never true, "
                   "the goal still true at the end; the path includes that tail (any fight search: an attack already committed lands after)")
    p.add_argument("--hold-actions", nargs="+", help="the tails tried in order, each cycled to N ticks (default: \".\" = stand still); "
                   "e.g. . R L U D UR UL DR DL")
    p.add_argument("--hold-no-goal", action="store_true", help="--hold: only --avoid must stay false, the goal need not hold at the end")
    p.set_defaults(fn=cmd_search)
    p = sp.add_parser("look"); p.add_argument("run"); p.add_argument("--at"); p.add_argument("--target", choices=("win", "wine")); p.add_argument("--scale", type=float, default=1.0); p.set_defaults(fn=cmd_look)
    p = sp.add_parser("strip"); p.add_argument("run"); p.add_argument("--from", dest="frm", type=int, required=True); p.add_argument("--to", type=int)
    p.add_argument("--every", type=int); p.add_argument("--cols", type=int, default=4); p.add_argument("--target", choices=("win", "wine")); p.set_defaults(fn=cmd_strip)
    p = sp.add_parser("verify"); p.add_argument("run"); p.add_argument("--real", action="store_true", help="also replay on the real device (Windows)"); p.set_defaults(fn=cmd_verify)
    p = sp.add_parser("check"); p.add_argument("run"); p.add_argument("task"); p.set_defaults(fn=cmd_check)
    p = sp.add_parser("lint"); p.add_argument("task"); p.add_argument("--on", nargs="+", required=True); p.set_defaults(fn=cmd_lint)
    p = sp.add_parser("file"); p.add_argument("run"); p.add_argument("name"); p.add_argument("note", nargs="*"); p.set_defaults(fn=cmd_file)
    p = sp.add_parser("port"); p.add_argument("run"); p.add_argument("name", nargs="?"); p.set_defaults(fn=cmd_port)
    p = sp.add_parser("accept", help="check → verify --real → file → bless → hybrid + standalone → archive the report")
    p.add_argument("run"); p.add_argument("name"); p.add_argument("note", nargs="*"); p.add_argument("--task")
    p.add_argument("--force", action="store_true", help="file even though the task check fails")
    p.add_argument("--no-real", action="store_true"); p.add_argument("--no-gates", action="store_true"); p.set_defaults(fn=cmd_accept)
    p = sp.add_parser("stop"); p.add_argument("run"); p.set_defaults(fn=cmd_stop)
    sp.add_parser("ls").set_defaults(fn=cmd_ls)
    # an expression may start with "-" (--score "-exitd"): hand it to argparse as --flag=VALUE
    argv, i = [], 1
    while i < len(sys.argv):
        t = sys.argv[i]
        if t in ("--goal", "--avoid", "--until", "--score", "--cell", "--watch", "--expr", "--where") and i + 1 < len(sys.argv) and sys.argv[i + 1].startswith("-") \
                and not sys.argv[i + 1].startswith("--"):
            argv.append(f"{t}={sys.argv[i + 1]}"); i += 2
        else:
            argv.append(t); i += 1
    a = ap.parse_args(argv)
    try:
        sys.exit(a.fn(a) or 0)
    except SessionEnded:
        rd = os.path.join(RUNS, getattr(a, "run", "") or "", "session.json")
        log, why = "", ""
        if os.path.exists(rd):
            s = json.load(open(rd))
            log = os.path.join(os.path.dirname(rd), ".ended.log")
            if sess_file(s, "rhost.log", log):
                tail = open(log, errors="replace").read()[-1500:]
                os.remove(log)
                why = " | ".join(l[:160] for l in tail.splitlines() if l.startswith(("game:", "exception", "FATAL", "RESULT")))
            log = f"{s.get('rundir')}\\rhost.log ({s.get('host', 'wine')})" if s.get("host") in ("win", "bunker") else os.path.join(s.get("rundir", ""), "rhost.log")
        print(f"the live session ended during this command: the game exited (EXIT on the title, a quit) or crashed.\n  {why or log}\n"
              "  The run is unchanged (its movie ends before this command); the next rd command replays it.", file=sys.stderr)
        sys.exit(3)




def bin_sha():
    """the build a session runs (a session of another build is rebuilt by replay)"""
    return RH.file_sha12(os.path.join(RH.BUILD, "rhost.exe"))


def session_args(movie, win, upto=None, src_trace=None, limit=None, idle=1800, standalone=False):
    """the rhost command line of a session (paths as the session's host sees them)"""
    ed = movie.edition or "enold"
    gd = f"{RH.WROOT}\\game\\{RH.GAMEDIR[ed]}"
    args = ["--exe", f"{gd}\\{RH.EXES[ed]}", "--game", gd, "--run", win, "--serve", "0", "--serve-idle", str(idle)] + skip_args()
    if standalone:
        args += ["--standalone"]
    if src_trace:
        args += ["--trace", win + "\\trace.jsonl"] + (["--serve-until", str(limit)] if limit else [])
    else:
        upto = movie.frame if upto is None else upto
        if upto > 0:
            args += ["--trace", win + "\\trace.jsonl", "--serve-until", str(upto)]
    return args



def store_user_dir(src):
    """a user\\ dir (statue saves + config.dat) → tests/scenarios/_saves/<digest>/ (user data, gitignored) → (digest, path)"""
    digest = RH.save_digest(src)
    dst = os.path.join(SCEN_SAVES, digest)
    if not os.path.isdir(dst):
        os.makedirs(SCEN_SAVES, exist_ok=True)
        tmp = dst + f".tmp{os.getpid()}"
        shutil.copytree(src, tmp)
        os.replace(tmp, dst)
    return digest, dst


def save_path(arg):
    """save:DIGEST (a prefix of a _saves dir) or a path"""
    if os.path.exists(arg):
        return os.path.abspath(arg)
    hits = [d for d in (os.listdir(SCEN_SAVES) if os.path.isdir(SCEN_SAVES) else []) if d.startswith(arg) and os.path.isdir(os.path.join(SCEN_SAVES, d))]
    if len(hits) != 1:
        raise SystemExit(f"save:{arg}: {'no' if not hits else 'ambiguous'} match in {SCEN_SAVES}")
    return os.path.join(SCEN_SAVES, hits[0])


def session_shots(r, frames, tag="look"):
    """PNGs of the movie at FRAMES (headless pixels, straight from a session's memory): the run's own session for its
    head, a side session (replaying from boot) for earlier frames → [local paths]"""
    out_dir = os.path.join(r.dir, "view")
    os.makedirs(out_dir, exist_ok=True)
    frames = sorted(set(frames))
    paths = []

    def grab(lv, s, f):
        name = f"{tag}_f{f:06d}.png"
        remote = (s["rundir"] if s.get("host", "wine") != "wine" else
                  f"{RH.WROOT}\\runs\\drive\\{os.path.basename(s['rundir'])}") + "\\" + name
        lv.cmd(f"shot {remote}")
        dst = os.path.join(out_dir, name)
        if os.path.exists(dst):
            os.remove(dst)
        if not sess_file(s, name, dst):
            raise SystemExit(f"shot {name}: not fetched from the session")
        paths.append(dst)
    head = [f for f in frames if f == r.movie.frame]
    early = [f for f in frames if f < r.movie.frame]
    if any(f > r.movie.frame for f in frames):
        raise SystemExit(f"frames past the movie's end ({r.movie.frame})")
    if head:
        grab(r.attach(), r.sess, head[0])
    if early:
        h = spawn_live(tag, r.movie, upto=early[0], idle=300)
        try:
            lv = Live(h["port"])
            at = early[0]
            for f in early:
                for a, b in r.movie.chunks(at, f):
                    lv.cmd("run " + r.movie.run_args(a, b))
                at = f
                grab(lv, h, f)
            lv.close()
        finally:
            kill_session(h)
    return paths



def gates(name, verbose=True):
    """the corpus gates for a filed scenario: bless the retail expectation (Windows real device: frame / tick / RNG /
    state / every frame's pixels), then the hybrid (every ported function replacing retail) and the standalone (no retail
    code) against it → [(gate, ok, line)]"""
    py = [sys.executable, os.path.join(RH.HERE, "rhost.py")]
    out = []
    for gate, extra in (("bless", ["--bless"]), ("hybrid", ["--check", "--hybrid"]), ("standalone", ["--check", "--standalone"])):
        p = subprocess.run(py + ["corpus", "--only", f"^{re.escape(name)}$"] + extra, capture_output=True, text=True)
        tail = [l for l in (p.stdout + p.stderr).splitlines() if l.strip()][-4:]
        ok = p.returncode == 0 and not any(re.search(r"REGRESSION|FAIL|NO RESULT", l) for l in tail)
        out.append((gate, ok, " | ".join(tail)))
        if verbose:
            print(f"{gate}: {'PASS' if ok else 'FAIL'}  {tail[-1] if tail else ''}")
        if gate == "bless" and not ok:
            break
    return out

if __name__ == "__main__":
    main()
