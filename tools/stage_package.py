#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, shutil, sys
from pathlib import Path

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--module", type=Path, required=True, help="Built xenon_game_module DLL/SO")
    ap.add_argument("--platform", choices=["windows-x64","linux-x64"], required=True)
    ap.add_argument("--output", type=Path, default=Path("dist/org.nimauria.project-gracemeria"))
    args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    out=args.output
    if out.exists(): shutil.rmtree(out)
    (out/"native"/args.platform).mkdir(parents=True)
    shutil.copy2(root/"xenon-module.json", out/"xenon-module.json")
    shutil.copy2(root/"README.md", out/"README.md")
    shutil.copy2(root/"LICENSE", out/"LICENSE")
    shutil.copytree(root/"revisions", out/"revisions")
    expected="xenon_game_module.dll" if args.platform=="windows-x64" else "libxenon_game_module.so"
    shutil.copy2(args.module, out/"native"/args.platform/expected)
    manifest=json.loads((out/"xenon-module.json").read_text(encoding="utf-8"))
    declared=manifest["nativeExtension"][args.platform]
    if Path(declared).name != expected:
        raise SystemExit(f"manifest path {declared!r} does not match staged file {expected!r}")
    print(out)
    return 0
if __name__=="__main__": raise SystemExit(main())
