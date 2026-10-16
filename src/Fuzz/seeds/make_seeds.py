"""Synthetic seed inputs for GrayWolf's fuzz targets (src/Fuzz/FuzzTargets.cpp).

Usage: py -3 make_seeds.py <target|all> <out dir>

Small, well-formed files built from the formats' layouts as GrayWolf reads them, so the fuzzers
start past the magic checks without shipping game content. Real files make better seeds: keep
those outside the repo (tools/fuzz.ps1 also reads <corpus>/<target>-real).
"""
import io
import json
import os
import struct
import sys
import zipfile
import zlib


def u8(v): return struct.pack("<B", v)
def u16(v): return struct.pack("<H", v)
def u32(v): return struct.pack("<I", v)
def u64(v): return struct.pack("<Q", v)
def f32(*v): return struct.pack("<%df" % len(v), *v)
def cstr(s): return s.encode() + b"\0"


def fastfile():
    zone = bytes(64)
    out = {}
    for magic, version in ((b"IWffu100", 5), (b"IWffu100", 276), (b"IWffu100", 387), (b"IWffu100", 473),
                           (b"IWffu100", 565), (b"IWff0100", 1), (b"IWff0100", 461), (b"IWff0100", 1619),
                           (b"IWffa100", 25), (b"S1ffu100", 5), (b"S1ffu100", 66), (b"S1ff0100", 1838),
                           (b"TAff0100", 147), (b"TAff0000", 593)):
        out["%s_%d.ff" % (magic.decode(), version)] = magic + u32(version) + zlib.compress(zone)
    out["TAFF.ff"] = b"TAFF" + u32(1) + u32(0x39CB44F1) + u32(4) + u32(0) + bytes(32)
    return out


def iwi_v6(fmt, w, h, payload):
    size = 28 + len(payload)
    return b"IWi" + u8(6) + u8(fmt) + u8(0) + u16(w) + u16(h) + u16(1) + u32(size) * 4 + payload


def iwi():
    return {
        "argb_4x4.iwi": iwi_v6(1, 4, 4, bytes(range(64))),
        "rgb_2x2.iwi": iwi_v6(2, 2, 2, bytes(12)),
        "dxt1_4x4.iwi": iwi_v6(0x0B, 4, 4, bytes(8)),
        "dxt5_4x4.iwi": iwi_v6(0x0D, 4, 4, bytes(16)),
        "v8_dxt1.iwi": b"IWi" + u8(8) + u8(0x0B) + u8(0) + u8(0) + u8(0) + u16(4) + u16(4) + u16(1) + u16(0)
                       + u32(36) * 4 + bytes(8),
        "v13_dxt1.iwi": b"IWi" + u8(0x0D) + u8(0x0B) + u8(0) + u16(4) + u16(4) + u16(1) + u16(0) + u32(40) * 8 + bytes(8),
    }


def iwd():
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("images/test_c.iwi", iwi()["dxt1_4x4.iwi"])
        z.writestr("images/test_n.iwi", iwi()["argb_4x4.iwi"])
        z.writestr("sound/test.wav", b"RIFF" + u32(36) + b"WAVEfmt " + u32(16) + u16(1) + u16(1) + u32(22050) + u32(44100) + u16(2) + u16(16) + b"data" + u32(0))
    return {"images.iwd": buf.getvalue()}


def sab():
    out = {}
    # version: (audio entry size, name entry size)
    for version, entry in ((0x4, 44), (0xA, 32), (0xE, 20), (0xF, 36), (0x11, 40), (0x15, 56), (0x15, 64)):
        names_at, entries_at = 0x300, 0x400
        head = u32(0x23585532) + u32(version) + u32(entry) + u32(16) + u32(64) + u32(2) + bytes(8)
        head += u64(0x600) + u64(entries_at) + u64(0x500)
        data = bytearray(0x600)
        data[:len(head)] = head
        name_slot = 0x24C if (version == 0x15 and entry == 0x38) else 0x250
        data[name_slot:name_slot + 8] = u64(names_at)
        for i, name in enumerate(("sound/test_a.wav", "sound/test_b.wav")):
            n = cstr(name)
            data[names_at + i * 128:names_at + i * 128 + len(n)] = n
        for i in range(2):
            e = bytearray(entry)
            e[0:4] = u32(0x1000 + i)
            e[4:8] = u32(16)
            data[entries_at + i * entry:entries_at + (i + 1) * entry] = e
        out["v%x_%d.sabl" % (version, entry)] = bytes(data)
    return out


def xpak():
    out = {}
    props = b"type: image\nwidth: 4\nheight: 4\nsize0: 16\nname: test_c\nformat: bc1_unorm\n"
    for version in (0xB, 0xD, 0x17):
        hash_at, index_at = 0x200, 0x300
        fields = [0, 0x400, 1, 0x400, 0x10, 1, hash_at, 24, 0, 0, 0, 1, index_at, 16 + len(props)]
        head = u32(0x4950414B) + u16(0) + u16(version) + b"".join(u64(v) for v in fields)
        data = bytearray(0x400)
        if version == 0xD:
            data[:24] = head[:24]
            data[24 + 288:24 + 288 + 96] = head[24:120]
        else:
            data[:len(head)] = head
        data[hash_at:hash_at + 24] = u64(0x1234) + u64(0x400) + u64(16)
        data[index_at:index_at + 16 + len(props)] = u64(0x1234) + u64(len(props)) + props
        out["v%x.xpak" % version] = bytes(data)
    return out


def ipak():
    data = bytearray(0x200)
    data[:16] = u32(0x4950414B) + u32(0x50000) + u32(0x200) + u32(2)
    data[16:48] = u32(1) + u32(0x100) + u32(32) + u32(2) + u32(2) + u32(0x180) + u32(64) + u32(1)
    data[0x100:0x120] = u64(0xABCD) + u32(0) + u32(16) + u64(0xBEEF) + u32(16) + u32(16)
    return {"images.ipak": bytes(data)}


def ifs():
    # MPQ-style header ("nifs"): too involved to build here; a bare header gets the parser started
    return {"header.ifs": b"nifs" + u32(0x20) + u32(0x1000) + u16(3) + u16(9) + bytes(0x200)}


def seanim():
    bones = ("root", "child")
    frames = 11
    head = b"SEAnim" + u16(1) + u16(0x1C) + u8(2) + u8(1) + u8(0x1 | 0x2 | 0x40) + u8(0) + u16(0)
    head += f32(30.0) + u32(frames) + u32(len(bones)) + u8(1) + bytes(3) + u32(1)
    body = b"".join(cstr(b) for b in bones) + u8(1) + u8(0)
    for _ in bones:
        body += u8(0)
        body += u8(2) + u8(0) + f32(0, 0, 0) + u8(10) + f32(5, 0, 0)
        body += u8(2) + u8(0) + f32(0, 0, 0, 1) + u8(10) + f32(0, 0.7071, 0, 0.7071)
    body += u8(5) + cstr("fire")
    return {"two_bones.seanim": head + body}


def semodel():
    head = b"SEModel" + u16(1) + u16(0x14) + u8(0x7) + u8(0x3) + u8(0xF) + u32(1) + u32(1) + u32(1) + bytes(3)
    body = cstr("tag_origin")
    body += u8(0) + struct.pack("<i", -1) + f32(0, 0, 0) + f32(0, 0, 0, 1) + f32(0, 0, 0) + f32(0, 0, 0, 1)
    body += u8(0) + u8(1) + u8(1) + u32(3) + u32(1)
    body += f32(0, 0, 0, 1, 0, 0, 0, 1, 0)
    body += f32(0, 0, 1, 0, 0, 1)
    body += f32(0, 0, 1) * 3
    body += bytes([255]) * 12
    body += (u8(0) + f32(1.0)) * 3
    body += u8(0) + u8(1) + u8(2)
    body += struct.pack("<i", 0)
    body += cstr("mtl_test") + u8(1) + cstr("test_c") + cstr("test_n") + cstr("")
    return {"triangle.semodel": head + body}


def sidecar():
    doc = {"format": "greyhound-materials", "version": 1, "game": "bo3", "adapter": "t7", "materials": [
        {"name": "mtl_test", "alpha": "blend", "alpha_cutoff": 0.5,
         "albedo": {"image": "test_c.dds"}, "normal": {"image": "test_n.dds", "encoding": "ag", "flip_y": True},
         "specular": {"image": "test_s.dds", "legacy": True, "scale": 2.0},
         "transmission": {"image": "test_t.dds", "color": [1, 0.5, 0.25]},
         "alpha_map": {"image": "test_a.dds", "low": 0.1, "high": 0.9, "uv_scale": [2, 2]},
         "tint": {"color": [1, 1, 1]}, "color_tint": {"mask": {"image": "m.dds"}, "color": [1, 0, 0]},
         "layers": [{"albedo": {"image": "layer_c.dds"}}], "roughness_value": 0.5, "metalness_value": 0.0,
         "techset": "lit", "sources": [{"path": "a.dds", "semantic": "0x00000001"}]}]}
    return {"model.materials.json": json.dumps(doc, indent=1).encode()}


TARGETS = {"fastfile": fastfile, "iwi": iwi, "iwd": iwd, "sab": sab, "xpak": xpak, "ipak": ipak,
           "ifs": ifs, "seanim": seanim, "semodel": semodel, "sidecar": sidecar}


def main():
    if len(sys.argv) != 3 or (sys.argv[1] != "all" and sys.argv[1] not in TARGETS):
        print(__doc__)
        print("targets:", " ".join(TARGETS))
        return 2
    names = list(TARGETS) if sys.argv[1] == "all" else [sys.argv[1]]
    for name in names:
        out = sys.argv[2] if len(names) == 1 else os.path.join(sys.argv[2], name)
        os.makedirs(out, exist_ok=True)
        for file, data in TARGETS[name]().items():
            with open(os.path.join(out, file), "wb") as fh:
                fh.write(data)
    return 0


if __name__ == "__main__":
    sys.exit(main())
