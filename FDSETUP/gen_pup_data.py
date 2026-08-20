#!/usr/bin/env python3
"""Generate pup_data.c from SCREENS/*.PUP files."""
import glob, os, sys

screens_dir = os.path.join(os.path.dirname(__file__), "SCREENS")
out_path    = os.path.join(os.path.dirname(__file__), "pup_data.c")

pups = sorted(glob.glob(os.path.join(screens_dir, "*.PUP")))
if not pups:
    sys.exit(f"No .PUP files found in {screens_dir}")

lines = [
    "/* Auto-generated from SCREENS/*.PUP - do not edit */",
    "/* Full binary content; DrawPup reads drawing data past the pup_t header */",
    "",
]
for path in pups:
    name = os.path.splitext(os.path.basename(path))[0].lower()
    data = open(path, "rb").read()
    hex_bytes = ", ".join(f"0x{b:02x}" for b in data)
    lines.append(f"char far {name}[] = {{{hex_bytes}}};")

with open(out_path, "w") as f:
    f.write("\n".join(lines) + "\n")

print(f"Written {out_path} ({len(pups)} entries)")
