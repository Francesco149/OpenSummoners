#!/usr/bin/env python3
"""api_check.py — is there API credit right now? The orchestrator asks before choosing its driver kind:
  credit → `tools/drive/agent.sh TASK --run R` (API-billed, parallel, budgeted); none → a Claude Code subagent on the
  session's own model (subscription). Never decided by a guess: one ~10-token request to the cheapest current model.

    tools/drive/agent.sh check [--max-age 900] [--json]

The API cannot report the prepaid balance, so a LEDGER does: `--set-balance USD` when credit is added (the Console's
figure); agent.py subtracts every turn's cost. "Available" = the probe succeeds AND the ledger's remainder covers
--need (default $2: a typical probe's budget). Without a ledger the probe alone decides (and says so).

Exit 0 = credit available, 4 = no / too little credit (billing 400, or the ledger below --need), 5 = no key, 3 = other error.
The verdict is cached in /mnt/ephemeral/opensummoners/drive/api_status.json for --max-age seconds (default 15 min):
asking costs ~$0.00001 a time, but campaigns ask often."""
import argparse, json, os, sys, time
import anthropic

CACHE = os.environ.get("RD_API_STATUS", "/mnt/ephemeral/opensummoners/drive/api_status.json")
LEDGER = os.environ.get("RD_API_LEDGER", "/mnt/ephemeral/opensummoners/drive/api_ledger.json")


def ledger():
    return json.load(open(LEDGER)) if os.path.exists(LEDGER) else None


def ledger_spend(usd):
    """agent.py: add a turn's cost (file-locked: drivers run side by side)"""
    import fcntl
    if not os.path.exists(LEDGER):
        return
    with open(LEDGER + ".lock", "w") as lk:
        fcntl.flock(lk, fcntl.LOCK_EX)
        L = json.load(open(LEDGER))
        L["spent"] = L.get("spent", 0.0) + usd
        json.dump(L, open(LEDGER + ".tmp", "w")); os.replace(LEDGER + ".tmp", LEDGER)


def remaining():
    L = ledger()
    return None if L is None else L["balance"] - L.get("spent", 0.0)


def key():
    k = os.environ.get("ANTHROPIC_API_KEY")
    p = os.path.expanduser("~/claude-platform.txt")
    if not k and os.path.exists(p):
        k = open(p).read().strip()
    return k


def probe():
    k = key()
    if not k:
        return {"ok": False, "code": 5, "why": "no API key (ANTHROPIC_API_KEY or ~/claude-platform.txt)"}
    c = anthropic.Anthropic(api_key=k, max_retries=2)
    try:
        c.messages.create(model="claude-haiku-5-5", max_tokens=8, messages=[{"role": "user", "content": "ok"}])
        return {"ok": True, "code": 0, "why": "a request succeeded"}
    except anthropic.BadRequestError as e:
        if "credit" in str(e).lower():
            return {"ok": False, "code": 4, "why": f"no credit: {e.message}"}
        return {"ok": False, "code": 3, "why": f"bad request: {e.message}"}
    except anthropic.AuthenticationError as e:
        return {"ok": False, "code": 5, "why": f"key rejected: {e.message}"}
    except anthropic.APIStatusError as e:
        if e.status_code == 402 or "credit" in str(e).lower():
            return {"ok": False, "code": 4, "why": f"no credit: {e.message}"}
        return {"ok": False, "code": 3, "why": f"API error {e.status_code}: {e.message}"}
    except anthropic.APIConnectionError as e:
        return {"ok": False, "code": 3, "why": f"network: {e}"}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--max-age", type=float, default=900); ap.add_argument("--json", action="store_true")
    ap.add_argument("--need", type=float, default=2.0, help="dollars a launch needs left in the ledger")
    ap.add_argument("--set-balance", type=float, help="record the account's balance now (USD, from the Console)")
    a = ap.parse_args()
    if a.set_balance is not None:
        os.makedirs(os.path.dirname(LEDGER), exist_ok=True)
        json.dump({"balance": a.set_balance, "spent": 0.0, "set_at": time.strftime("%Y-%m-%d %H:%M")}, open(LEDGER, "w"))
        print(f"ledger: balance ${a.set_balance:.2f} recorded")
    st = None
    if os.path.exists(CACHE):
        st = json.load(open(CACHE))
        if time.time() - st.get("t", 0) > a.max_age:
            st = None
    if st is None:
        st = dict(probe(), t=time.time())
        os.makedirs(os.path.dirname(CACHE), exist_ok=True)
        json.dump(st, open(CACHE, "w"))
    rem = remaining()
    code, ok, why = st["code"], st["ok"], st["why"]
    if ok and rem is not None and rem < a.need:
        code, ok, why = 4, False, f"ledger: ${rem:.2f} left < ${a.need:.2f} needed"
    out = dict(st, ok=ok, code=code, why=why, remaining=rem)
    led = f"; ledger ${rem:.2f} left" if rem is not None else "; no ledger (--set-balance USD): the probe alone decided"
    print(json.dumps(out) if a.json else f"API credit: {'AVAILABLE' if ok else 'NONE'} ({why}{led}; probed {time.time() - st['t']:.0f}s ago)")
    return code


if __name__ == "__main__":
    sys.exit(main())
