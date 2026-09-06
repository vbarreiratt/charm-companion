#!/usr/bin/env python3
"""Build (cached by mtime) and run render_reference.cpp against the real
production C++ source, then convert its raw RGB565 dump to a PNG.

See docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md.
"""
import argparse
import glob
import os
import subprocess
import sys

import numpy as np
from PIL import Image

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOL_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(TOOL_DIR, "render_reference_bin")

DISPLAY_WIDTH = 466
DISPLAY_HEIGHT = 466


def _source_files():
    files = glob.glob(os.path.join(REPO_ROOT, "src", "**", "*.cpp"), recursive=True)
    return [f for f in files if os.path.basename(f) != "main.cpp"]


def build(force=False):
    sources = _source_files() + [os.path.join(TOOL_DIR, "render_reference.cpp")]
    newest_source = max(os.path.getmtime(f) for f in sources)
    if not force and os.path.exists(BIN_PATH) and os.path.getmtime(BIN_PATH) > newest_source:
        return
    cmd = [
        "g++", "-std=c++17", "-O1",
        "-I", os.path.join(REPO_ROOT, "src"),
        "-I", os.path.join(REPO_ROOT, "config"),
        "-I", REPO_ROOT,
        *sources,
        "-o", BIN_PATH,
    ]
    subprocess.run(cmd, check=True)


def rgb565_to_rgb888(raw_bytes):
    values = np.frombuffer(raw_bytes, dtype="<u2").reshape(DISPLAY_HEIGHT, DISPLAY_WIDTH)
    r5 = (values >> 11) & 0x1F
    g6 = (values >> 5) & 0x3F
    b5 = values & 0x1F
    r8 = (r5 * 255 + 15) // 31
    g8 = (g6 * 255 + 31) // 63
    b8 = (b5 * 255 + 15) // 31
    return np.dstack([r8, g8, b8]).astype(np.uint8)


def render(target, out_png, touch=False, update_ms=0, force_build=False):
    build(force=force_build)
    raw_path = out_png + ".raw"
    cmd = [BIN_PATH, target, "--out", raw_path]
    if touch:
        cmd.append("--touch")
    if update_ms:
        cmd += ["--update-ms", str(update_ms)]
    subprocess.run(cmd, check=True)
    with open(raw_path, "rb") as f:
        raw = f.read()
    os.remove(raw_path)
    rgb = rgb565_to_rgb888(raw)
    Image.fromarray(rgb, mode="RGB").save(out_png)
    return out_png


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", choices=["home", "planet", "eye"])
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--touch", action="store_true")
    parser.add_argument("--update-ms", type=int, default=0)
    parser.add_argument("--force-build", action="store_true")
    args = parser.parse_args()
    path = render(args.target, args.out, touch=args.touch, update_ms=args.update_ms, force_build=args.force_build)
    print(f"wrote {path}")


if __name__ == "__main__":
    sys.exit(main())
