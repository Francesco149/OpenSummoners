#!/usr/bin/env python3
"""api_check.py — is there API credit right now? The orchestrator asks before choosing its driver kind:
  credit → `tools/drive/agent.sh TASK --run R` (API-billed, parallel, budgeted); none → a Claude Code subagent on the
  session's own model (subscription). Never decided by a guess: one ~10-token request to the cheapest current model.

    tools/drive/agent.sh check [--max-age 900] [--json]

The API key is shared with other projects, so no local ledger can know the balance: the probe decides, and a driver
that drains the credit mid-run exits 4 at once (agent.py) — the orchestrator then hands its task to a subagent.

Exit 0 = credit available, 4 = no credit (billing 400 "credit balance is too low"), 5 = no key, 3 = other error.
The verdict is cached in /mnt/ephemeral/opensummoners/drive/api_status.json for --max-age seconds (default 15 min):
asking costs ~$0.00001 a time, but campaigns ask often."""
import argparse, json, os, sys, time
import anthropic

CACHE = os.environ.get("RD_API_STATUS", "/mnt/ephemeral/opensummoners/drive/api_status.json")


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
    a = ap.parse_args()
    st = None
    if os.path.exists(CACHE):
        st = json.load(open(CACHE))
        if time.time() - st.get("t", 0) > a.max_age:
            st = None
    if st is None:
        st = dict(probe(), t=time.time())
        os.makedirs(os.path.dirname(CACHE), exist_ok=True)
        json.dump(st, open(CACHE, "w"))
    print(json.dumps(st) if a.json else f"API credit: {'AVAILABLE' if st['ok'] else 'NONE'} ({st['why']}; checked {time.time() - st['t']:.0f}s ago)")
    return st["code"]


if __name__ == "__main__":
    sys.exit(main())
