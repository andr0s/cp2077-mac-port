"""Merge reviewed address proposals into a copy of RED4ext.SDK-macos's address database.

usage: python3 -I merge.py <sdk db.json> <proposals.json> <out db.json>

proposals.json: [{"hash": "123", "offset": "1:0xABC", "name": "...", "evidence": "..."}, ...]
Each proposal sets the entry's offset and marks it verified (RED4ext resolves only verified entries). The SDK file is
not modified. Prints one line per change.
"""
import json
import re
import sys

OFFSET = re.compile(r"^[1-3]:0x[0-9A-F]+$")
TEXT_END = 0x6DE0000  # __TEXT size: __DATA_CONST starts at 0x106DE0000


def merge(db, proposals):
    by_hash = {e["hash"]: e for e in db["Addresses"]}
    changes = []
    for p in proposals:
        h, off = str(p["hash"]), p["offset"].upper().replace("0X", "0x")
        if not h.isdigit() or not OFFSET.match(off):
            raise ValueError(f"bad proposal {p}")
        if off.startswith("1:") and int(off[2:], 16) >= TEXT_END:
            raise ValueError(f"{p.get('name', h)}: {off} is outside __TEXT")
        entry = by_hash.get(h)
        if entry is None:
            entry = {"hash": h}
            db["Addresses"].append(entry)
            by_hash[h] = entry
            old = "absent"
        else:
            old = entry.get("offset", "none") + (" verified" if entry.get("verified") else "")
        entry["offset"] = off
        entry["verified"] = True
        changes.append(f"{p.get('name', '?')} {h}: {old} -> {off} verified")
    db["Addresses"].sort(key=lambda e: int(e["hash"]))
    entries = db["Addresses"]
    resolved = sum(1 for e in entries if e.get("offset") and not e["offset"].endswith(":0x0"))
    db["stats"] = {
        "total": len(entries),
        "resolved": resolved,
        "unresolved": len(entries) - resolved,
        "verified": sum(1 for e in entries if e.get("verified")),
    }
    return changes


def main():
    sdk, proposals, out = sys.argv[1:4]
    with open(sdk) as f:
        db = json.load(f)
    with open(proposals) as f:
        props = json.load(f)
    for line in merge(db, props):
        print(line)
    with open(out, "w") as f:
        json.dump(db, f, indent=2)
        f.write("\n")


if __name__ == "__main__":
    main()
