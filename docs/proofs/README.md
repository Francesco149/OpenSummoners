# Proofs — human-verifiable RE results to share with the community

These are **occasional deliverables**, written for a *human* to reproduce — not for the
model to read back (our function annotation + host tests are the machine source of
truth; see CLAUDE.md). A proof earns a file here only when:

1. We can **100% prove** the claim — a host-tested port, a bit-exact consumption assert, or
   a live Frida confirmation. **Never** a guess about a function/data address or format.
2. The result is **missing or marked uncertain** in the community *SotES Data Formats &
   Values* spreadsheet (see `../ods-crossref.md`), so publishing it is a contribution.
3. A person with their **own legal copy** of the game can reproduce it from the recipe —
   the repo ships no game bytes; the proof references the user's own files / our committed
   tools (which also ship no assets).

## Template
Each proof file follows this shape:

```
# <claim, one line>

## Claim
The single, falsifiable statement being proven (addresses/offsets/values).

## Why this is proven, not guessed
What in our work establishes it: the ported+tested function (FUN_<va>), the host test,
the exact-consumption assertion, or the live capture. Cite the finding doc.

## Reproduce it yourself
Numbered steps a human runs against their own copy. Prefer a committed tool + a
hand-checkable byte-level path. State the EXACT expected output.

## What you should see
The concrete bytes/values + how to read them. Include the invariant that makes it
trustworthy (e.g. "the parse consumes the resource exactly — zero trailing bytes").

## ods status
Which sheet/cell this fills or corrects, and what was there before.
```

## Published
- [`map-data-format.md`](map-data-format.md) — the in-game map DATA resource format
  (absent from the ods; bit-exact consumption proof).
