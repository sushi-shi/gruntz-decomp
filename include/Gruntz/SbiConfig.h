// SbiConfig.h - the ONE shape of the status-bar-item CONFIG-HOST family. The
// vslot-11 item setup (CSBI_Image::SetupImage 0xe6c80, CSBI_RectOnly::ConfigureRect
// 0xe72f0, CSBI_MenuItem::ResolveFrame 0xe81e0) is handed a "config host" object as
// arg2. Every TU that touches it previously carried a divergent per-TU view
// (CImageCfgHost/CImageCfgMap/CImageCfgRecord, CMiCfgHost/CMiWordMap/CMiCueRec,
// CSbiCfgHost/CSbiCfgObj/CSbiCfgMap/CSbiCfgRecord, CGooHost/CGooPool); this is the
// single reconstructed layout, proven by shared retail method RVAs and offsets:
//
//   * the lookup map is the SAME engine function in every view: CMapWordToOb::
//     Lookup @ 0x1b8008 (reloc-masked, no body), reached as (host->m_10 + 0x10);
//   * the looked-up record is the SAME shape everywhere: a frame/value table at
//     +0x14 indexed by frame, and a [+0x64 .. +0x68] valid-frame range;
//   * the config host also exposes a draw-surface pool at +0x1c (CSBI_WellGoo's
//     dtor returns its owned surface there via RemoveItemA @ 0x142160).
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_SBICONFIG_H
#define GRUNTZ_SBICONFIG_H

#include <rva.h>

struct CSbiConfigRecord; // the value the lookup yields (defined below)

// CMapWordToOb::Lookup (engine 0x1b8008, __thiscall, ret 8): key -> *out record.
// Modeled with NO body so the `ecx=<map>; call 0x1b8008` shape reloc-masks.
SIZE_UNKNOWN(CSbiConfigMap);
struct CSbiConfigMap {}; // MFC CMapStringToPtr (Lookup @0x1b8008); cast at each call

// The registry object held at config-host+0x10: the CMapWordToOb map is embedded
// at ITS +0x10. Accessing `host->m_10->m_10map` yields the `[host+0x10]+0x10`
// map `this` the lookup uses (no raw-offset cast needed).
SIZE_UNKNOWN(CSbiConfigReg);
struct CSbiConfigReg {
    char m_pad0[0x10];
    CSbiConfigMap m_10map; // +0x10  embedded lookup map
};

// The keyed config record: m_14 = frame/value table (i32*, indexed by frame),
// [m_64, m_68] = the valid-frame range (m_64 doubles as the default frame).
SIZE_UNKNOWN(CSbiConfigRecord);
struct CSbiConfigRecord {
    char m_pad0[0x14];
    i32* m_14; // +0x14  frame/value table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  range lo / default frame
    i32 m_68; // +0x68  range hi
};

// The draw-surface pool held at config-host+0x1c: RemoveItemA (0x142160,
// __thiscall) frees one held surface. Reloc-masked (no body).
SIZE_UNKNOWN(CSbiSurfacePool);
struct CSbiSurfacePool {};

// The config host handed to the vslot-11 item setup (arg2). +0x10 -> the registry
// object whose embedded CMapWordToOb map lives at ITS +0x10 (reach the map as
// `host->m_10->m_10map`, which lowers to `[host+0x10]+0x10`); +0x1c -> the pool.
SIZE_UNKNOWN(CSbiConfigHost);
struct CSbiConfigHost {
    char m_pad0[0x10];
    CSbiConfigReg* m_10; // +0x10  registry object (map embedded at +0x10)
    char m_pad14[0x1c - 0x14];
    CSbiSurfacePool* m_1c; // +0x1c  draw-surface pool
};

#endif // GRUNTZ_SBICONFIG_H
