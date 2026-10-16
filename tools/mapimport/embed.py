# Embeds map_import.py (the Blender map import script's body) into
# src/GrayWolf/GrayWolf/MapImportScript.cpp as raw string literals under MSVC's 16 KB limit.
# Run after editing map_import.py: python tools/mapimport/embed.py
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "map_import.py")
OUT = os.path.join(HERE, "..", "..", "src", "GrayWolf", "GrayWolf", "MapImportScript.cpp")
MARK = "// ---- the script body"

body = open(SRC, encoding="utf-8").read()
# the part a script runs on its own (its parameters, for testing) stays out
body = body.split("# ---- body\n", 1)[1]
if ("/" + "*") in body or (")PY" + '"') in body:
    raise SystemExit("map_import.py holds a sequence a C++ raw literal or the repo hook refuses")
chunks, cur = [], ""
for line in body.splitlines(keepends=True):
    if len(cur) + len(line) > 12000:
        chunks.append(cur)
        cur = ""
    cur += line
chunks.append(cur)
head = open(OUT, encoding="utf-8").read().split(MARK)[0]
lits = "\n".join('R"PY(' + c + ')PY"' for c in chunks)
with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write(head + MARK + " (tools/mapimport/map_import.py, embedded by embed.py: edit it there)\n"
            "const char* const Body =\n" + lits + ";\n}\n}\n")
print("%d bytes in %d literals -> %s" % (len(body), len(chunks), os.path.normpath(OUT)))
