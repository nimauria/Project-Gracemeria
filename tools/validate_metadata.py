#!/usr/bin/env python3
from __future__ import annotations
import csv, json, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REV = ROOT / "revisions" / "usa-retail-tu0"

def load(name: str):
    with (REV / name).open(encoding="utf-8") as f:
        return json.load(f)

def fail(msg: str):
    raise SystemExit(f"metadata validation failed: {msg}")

def main() -> int:
    manifest = json.loads((ROOT / "xenon-module.json").read_text(encoding="utf-8"))
    rev = load("revision.json")
    hints = load("analysis/hints.json")
    hooks = load("analysis/hooks.json")
    repl = load("analysis/native-replacements.json")
    patches = load("compatibility/patches.json")
    content = load("content/content.json")

    if manifest.get("id") != "org.nimauria.project-gracemeria": fail("module id")
    if "4E4D07D1" not in manifest.get("gameIds", []): fail("title id")
    identities = manifest.get("content", {}).get("identities", [])
    if not any("531C30BE" in i.get("mediaIds", []) for i in identities): fail("US media id")
    if rev["identity"]["titleId"] != "4E4D07D1": fail("revision title")
    if rev["identity"]["mediaIds"] != ["531C30BE"]: fail("revision media")
    if manifest.get("regions") != ["NTSC-U"]: fail("only the verified US revision may be advertised")
    tu = manifest.get("titleUpdates", {})
    if tu.get("owner") != "Project Xenon XEX Loader V2": fail("title-update ownership")
    if tu.get("supportedEffectiveProfiles") != []: fail("unverified TU profile advertised")
    if hints["revision"] != rev["id"]: fail("hint revision scope")
    if len(hints["indirectCallSites"]) != 2: fail("indirect calls")
    if len(hints["knownSymbols"]) != 18: fail("active symbol count")
    if len(repl["mappings"]) != 16: fail("native replacement candidates")
    if any(m.get("enabled") for m in repl["mappings"]): fail("generic replacements must not be module-owned")
    if len(hooks["hooks"]) != 34: fail("hook classification coverage")
    if any(h.get("enabled") for h in hooks["hooks"]): fail("legacy ReXGlue hooks must not be enabled")
    if content["dlc"]["owner"] != "Xenon content/STFS subsystem": fail("DLC ownership")
    dlc = manifest.get("dlc", [])
    if len(dlc) != 65: fail("installed manifest DLC catalogue coverage")
    dlc_ids = [item.get("id") for item in dlc]
    if len(set(dlc_ids)) != len(dlc_ids): fail("duplicate DLC identifiers")
    if any(item.get("contentIds") for item in dlc): fail("unverified STFS content IDs must not be invented")
    if any(item.get("identityStatus") != "pending-stfs-header-verification" for item in dlc):
        fail("DLC identity verification status")
    if not patches["patches"]: fail("patch classification")

    with (REV / "analysis" / "legacy-symbols.csv").open(newline="", encoding="utf-8") as f:
        rows=list(csv.DictReader(f))
    if len(rows) < 10000: fail("legacy symbol map unexpectedly incomplete")
    if any(r["activation"] != "reference-only" for r in rows): fail("legacy symbols must stay reference-only")

    print(f"metadata ok: {len(rows)} legacy function records, 34 hooks, 16 native replacement candidates")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
