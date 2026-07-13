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
//   * the draw-surface pool (RemoveItemA @ 0x142160) is CGooGameMgr's +0x1c, not
//     the host's (SBI_WellGoo reaches it through (CGooGameMgr*)m_24).
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_SBICONFIG_H
#define GRUNTZ_SBICONFIG_H

#include <rva.h>
#include <Gruntz/GameRegistry.h> // CSpriteFactoryHolder - the real config-host class
#include <Gruntz/ResMgr.h>       // CImageRegistry (host->m_10) + its m_10map

struct CSbiConfigRecord; // the value the lookup yields (defined below)

// The registry object held at config-host+0x10: its name->record map is embedded at
// ITS +0x10. Accessing `host->m_10->m_10map` yields the `[host+0x10]+0x10` map `this`
// the lookup uses (no raw-offset cast needed).
//
// THE MAP IS ::CMapStringToOb, NOT CMapStringToPtr (mfc_class + disasm, 2026-07-13). The
// note that used to sit here had BOTH halves of the pairing inverted - it called 0x1b8008
// "CMapStringToPtr::Lookup" and 0x1b8438 "CMapStringToOb". The binary names the bands from
// each .obj ctor's own vtable stamp:
//     0x1b8008 = CMapStringToOb::Lookup   band [0x1b7e17, 0x1b8247)  vtbl 0x1eafd4
//     0x1b8438 = CMapStringToPtr::Lookup  band [0x1b8247, 0x1b85b1)  vtbl 0x1eb014
// and CSBI_GruntMachine::BuildResourceTabStatusBar (0xe8a70) / CSBI_MenuItem::ResolveFrame
// (0xe81e0) both `call 0x1b8008` - i.e. CMapStringToOb. Declaring CMapStringToPtr bound the
// WRONG routine (mfc_class --audit WRONG-CLASS; reloc-masked, so objdiff showed nothing).
// The four map classes are byte-identical (no COMDAT fold - MSVC5 has no /OPT:ICF), which
// is why every FID row there is AMBIG.  Ask the binary:
//     python -m gruntz.analysis.mfc_class 0x1b8008
// FOLDED (2026-07-13, Fable lane): the "config host" IS the world holder
// CSpriteFactoryHolder (<Gruntz/GameRegistry.h>) - the tree itself already cast
// `(host)g_gameReg->m_world` at two StatusBarTabBuilders sites, and its +0x10
// "registry" is the holder's CImageRegistry (<Gruntz/ResMgr.h>), whose embedded
// name map is the SAME m_10map at the SAME +0x10. The former CSbiConfigHost /
// CSbiConfigReg shells are gone; consumers include GameRegistry.h + ResMgr.h.

// The keyed config record: m_14 = frame/value table (i32*, indexed by frame),
// [m_64, m_68] = the valid-frame range (m_64 doubles as the default frame).
// (Layout-identical to <Image/ImageSet.h> CImageSet - m_frames @+0x14,
// m_minIndex/m_maxIndex @+0x64/+0x68 - the looked-up record IS a sprite set;
// fold candidate for the imageset owner lane.)
SIZE_UNKNOWN(CSbiConfigRecord);
struct CSbiConfigRecord {
    char m_pad0[0x14];
    i32* m_14; // +0x14  frame/value table (== CImageSet::m_frames)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  range lo / default frame (== CImageSet::m_minIndex)
    i32 m_68; // +0x68  range hi               (== CImageSet::m_maxIndex)
};

// The draw-surface pool (CGooGameMgr::m_1c owner): RemoveItemA (0x142160,
// __thiscall) frees one held surface. Reloc-masked (no body).
SIZE_UNKNOWN(CSbiSurfacePool);
struct CSbiSurfacePool {};

#endif // GRUNTZ_SBICONFIG_H
