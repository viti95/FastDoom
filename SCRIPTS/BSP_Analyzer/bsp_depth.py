#!/usr/bin/env python3
"""
doom_bsp_depth_all.py

Recorre todos los mapas de un WAD Doom y calcula
la profundidad máxima del árbol BSP de cada uno.

Compatible con:
- Doom
- Doom II
- Ultimate Doom
- PWADs clásicos
- formato vanilla NODES

Uso:
    python doom_bsp_depth_all.py doom2.wad
"""

import re
import struct
import sys
from collections import namedtuple

# ============================================================
# Estructura de nodo BSP vanilla Doom
# ============================================================

NODE_STRUCT = struct.Struct("<hhhhhhhhhhhhHH")

Node = namedtuple("Node", [
    "x", "y",
    "dx", "dy",
    "bbox",
    "children"
])

SSECTOR_FLAG = 0x8000

# ============================================================
# Utilidades WAD
# ============================================================

class WadFile:
    def __init__(self, path):
        self.fp = open(path, "rb")
        self._read_header()
        self._read_directory()

    def _read_header(self):
        self.fp.seek(0)

        ident, numlumps, infotableofs = struct.unpack(
            "<4sII",
            self.fp.read(12)
        )

        self.ident = ident.decode("ascii")
        self.numlumps = numlumps
        self.infotableofs = infotableofs

    def _read_directory(self):
        self.directory = []

        self.fp.seek(self.infotableofs)

        for _ in range(self.numlumps):
            filepos, size, name = struct.unpack(
                "<II8s",
                self.fp.read(16)
            )

            name = name.rstrip(b"\0").decode("ascii")

            self.directory.append({
                "name": name,
                "filepos": filepos,
                "size": size
            })

    def get_lump_data(self, lump_index):
        lump = self.directory[lump_index]

        self.fp.seek(lump["filepos"])

        return self.fp.read(lump["size"])

# ============================================================
# BSP
# ============================================================

def parse_nodes(data):
    nodes = []

    for i in range(0, len(data), NODE_STRUCT.size):

        chunk = data[i:i + NODE_STRUCT.size]

        if len(chunk) != NODE_STRUCT.size:
            break

        raw = NODE_STRUCT.unpack(chunk)

        node = Node(
            x=raw[0],
            y=raw[1],
            dx=raw[2],
            dy=raw[3],
            bbox=raw[4:12],
            children=(raw[12], raw[13])
        )

        nodes.append(node)

    return nodes


def is_subsector(child_id):
    return (child_id & SSECTOR_FLAG) != 0


def child_index(child_id):
    return child_id & 0x7FFF


def compute_depth(nodes, node_index):
    node = nodes[node_index]

    depths = []

    for child in node.children:

        if is_subsector(child):
            depths.append(1)
        else:
            depths.append(
                1 + compute_depth(nodes, child_index(child))
            )

    return max(depths)

# ============================================================
# Detección de mapas
# ============================================================

MAP_PATTERNS = [
    re.compile(r"^MAP\d\d$"),   # Doom II
    re.compile(r"^E\dM\d$"),    # Ultimate Doom
]


def is_map_marker(name):
    for pattern in MAP_PATTERNS:
        if pattern.match(name):
            return True
    return False


def find_all_maps(wad):
    maps = []

    for i, lump in enumerate(wad.directory):

        name = lump["name"]

        if is_map_marker(name):
            maps.append((name, i))

    return maps

# ============================================================
# Main
# ============================================================

def main():

    if len(sys.argv) != 2:
        print("Uso:")
        print("  python doom_bsp_depth_all.py <wadfile>")
        sys.exit(1)

    wad_path = sys.argv[1]

    wad = WadFile(wad_path)

    maps = find_all_maps(wad)

    if not maps:
        print("No se encontraron mapas.")
        sys.exit(1)

    print("")
    print(f"WAD: {wad_path}")
    print("")
    print(f"{'MAPA':<10} {'PROFUNDIDAD BSP':>18}")
    print("-" * 30)

    for map_name, map_index in maps:

        #
        # NODES normalmente está en:
        #
        # MAPxx
        # THINGS
        # LINEDEFS
        # SIDEDEFS
        # VERTEXES
        # SEGS
        # SSECTORS
        # NODES
        #

        nodes_lump_index = map_index + 7

        if nodes_lump_index >= len(wad.directory):
            print(f"{map_name:<10} ERROR")
            continue

        lump = wad.directory[nodes_lump_index]

        if lump["name"] != "NODES":
            print(f"{map_name:<10} SIN NODES")
            continue

        try:
            data = wad.get_lump_data(nodes_lump_index)

            nodes = parse_nodes(data)

            if not nodes:
                print(f"{map_name:<10} VACIO")
                continue

            #
            # El último nodo es la raíz
            #

            root_index = len(nodes) - 1

            depth = compute_depth(nodes, root_index)

            print(f"{map_name:<10} {depth:>18}")

        except Exception as e:
            print(f"{map_name:<10} ERROR ({e})")

    print("")


if __name__ == "__main__":
    main()
