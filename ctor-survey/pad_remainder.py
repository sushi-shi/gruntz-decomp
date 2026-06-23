#!/usr/bin/env python3
"""Tail-pad the 12 incomplete graduated classes to their known real size so their
SIZE() assert passes (build green), and fix the CGameMgr namespace. Pad = real -
current(modeled), inserted as the last member before the class's closing brace;
it sits beyond the fields matched code references, so it's matching-safe."""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]

# class -> (defining file, pad bytes)   pad = real_size - clang-reported current sizeof
PADS = {
    "CRainCloud":              ("src/Gruntz/GameObjectCtors.cpp", 0xf0),
    "CUFO":                    ("src/Gruntz/GameObjectCtors.cpp", 0xf0),
    "CGrunt":                  ("include/Gruntz/Grunt.h",          0x78),
    "CMapMgr":                 ("include/Gruntz/MapMgr.h",         0x34),
    "CTileTriggerSwitchLogic": ("include/Gruntz/TileTriggerSwitchLogic.h", 0x4),
    "CGruntPowerupSprite":     ("src/Gruntz/UserLogic.cpp",        0x1c),
    "CGruntSelectedSprite":    ("src/Gruntz/UserLogic.cpp",        0x18),
    "CSecretLevelTrigger":     ("src/Gruntz/UserLogic.cpp",        0x14),
    "CTeleporter":             ("src/Gruntz/UserLogic.cpp",        0x8),
    "CTileTrigger":            ("src/Gruntz/UserLogic.cpp",        0x14),
    "CVoiceTrigger":           ("src/Gruntz/UserLogic.cpp",        0x14),
    "CBoomerang":              ("src/Stub/CBoomerang.cpp",         0x108),
}


def class_close(text, cls):
    """index of the class's closing '}' (matching its definition's first '{')."""
    m = re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{", text)
    if not m:
        return None
    i = text.index("{", m.start()); depth = 0
    while i < len(text):
        depth += (text[i] == "{") - (text[i] == "}")
        if depth == 0:
            return i
        i += 1
    return None


for cls, (rel, pad) in PADS.items():
    p = REPO / rel
    t = p.read_text()
    if "m_size_pad" in t and re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{[^}]*m_size_pad", t):
        print(f"skip {cls}: already padded"); continue
    c = class_close(t, cls)
    if c is None:
        print(f"MISS {cls}: class def not found in {rel}"); continue
    ins = f"    char m_size_pad[0x{pad:x}]; // tail-pad to real size (gruntz.analysis.news); layout TBD\n"
    p.write_text(t[:c] + ins + t[c:])
    print(f"padded {cls:26} +0x{pad:<5x} {rel}")

# CGameMgr namespace fix
ga = REPO / "src/Wap32/GameApp.cpp"
t = ga.read_text()
if "SIZE(CGameMgr," in t:
    ga.write_text(t.replace("SIZE(CGameMgr,", "SIZE(WAP32::CGameMgr,"))
    print("fixed  CGameMgr -> WAP32::CGameMgr")
