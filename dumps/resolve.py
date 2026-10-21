#!/usr/bin/env python3
"""Check which quoted #include files the given TUs cannot resolve.

Usage: resolve.py <jni-root> <includedir>...
Reports every "..." include the compiler could not find via the TU's own
directory or one of the -I directories, which is what ndk-build passes.
"""
import os
import re
import sys

root = sys.argv[1]
incdirs = [os.path.abspath(p) for p in sys.argv[2:]]
inc_re = re.compile(r'^\s*#\s*include\s+"([^"]+)"')

TUS = [
    "ImGui/imgui.cpp",
    "ImGui/imgui_draw.cpp",
    "ImGui/imgui_widgets.cpp",
    "ImGui/imgui_tables.cpp",
    "ImGui/backends/imgui_impl_opengl3.cpp",
    "menu/neverlose_menu.cpp",
    "menu/hexsync_intro.cpp",
    "menu/hexsync_glow.cpp",
    "menu/gui.cpp",
    "menu/blur.cpp",
    "menu/nl_ui.cpp",
    "menu/nl_bindings.cpp",
    "menu/nl_embedded_assets.cpp",
    "Main.cpp",
]

bad = 0
for tu in TUS:
    path = os.path.join(root, tu)
    if not os.path.exists(path):
        print("MISSING TU   %s" % tu)
        bad += 1
        continue
    here = os.path.dirname(os.path.abspath(path))
    for line in open(path, encoding="utf-8", errors="replace"):
        m = inc_re.match(line)
        if not m:
            continue
        name = m.group(1)
        found = [d for d in [here] + incdirs if os.path.exists(os.path.join(d, name))]
        if not found:
            print("UNRESOLVED   %-42s <- %s" % (name, tu))
            bad += 1

print("unresolved: %d" % bad)
sys.exit(1 if bad else 0)
