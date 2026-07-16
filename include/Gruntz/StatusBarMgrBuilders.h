// StatusBarMgrBuilders.h - the include set the CStatusBarMgr per-tab builder
// (LoadTabSprites, StatusBarMgr.cpp) needs: the canonical CStatusBarMgr + the canonical
// SBI leaf classes it instantiates.
//
// This header USED to define a parallel `CSBI_*` hierarchy of its own, rooted at a
// fabricated base `CSbConfigItem` (<Gruntz/SbConfigItem.h>, now deleted). That base
// declared FIFTEEN virtuals - eleven of them body-less placeholders whose only job was to
// push `Configure` out to slot +0x2c and `ConfigureEx` to +0x34 - and every leaf derived
// from it, so cl emitted a 15-slot (60 B) vtable for EVERY widget. Retail's are 11/12/13/15:
//
//     CStatusBarItem 11 | CSBI_RectOnly 11 | CSBI_Image 12 | CSBI_ImageSet 13
//     CSBI_MenuItem 12  | CSBI_WellGoo 12  | CSBI_WarlordHead 13
//     CSBI_ImageSetAni 15 | CSBI_StatzTabArrow 15 | CSBI_SideTab 11   (RTTI, sema class)
//
// So `??_7CSBI_Image@@6B@` was emitted at 48 B by the seven leaf TUs and at 60 B here -
// ONE mangled name, two byte-lengths. MSVC5 keeps a single COMDAT per name and DISCARDS
// the rest: whichever the linker picked, some TU got a vtable of the wrong length, and a
// virtual call past the truncation point dispatches into whatever follows it. That is a
// shipped dispatch/teardown bug, not a scoring artifact.
//
// The fabricated base was never needed: its placeholder slots line up EXACTLY with the
// real chain's, arity and all - Configure(11 dwords) IS CSBI_Image::SetupImage (slot 11),
// ConfigureEx(14) IS CSBI_ImageSetAni::Init (slot 13), ApplyDir(5) IS SetRange (slot 14).
// The builders now instantiate the canonical classes directly; `new CSBI_X` auto-stamps
// the right retail vtable, at the right length, identical in every TU.
#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                 // CPtrList (embedded tab lists in CStatusBarMgr)
#include <Gruntz/SbRect.h>       // the by-value geometry rect the setup virtuals take
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr
#include <Gruntz/SbiConfig.h>    // canonical CDDrawSurfaceMgr (the setup virtuals' arg2)

// The canonical SBI leaves the tab builder instantiates. Each carries its own type tag in
// its ctor (3/4/5/6/7/8/9/0xb) over the out-of-line CSBI_RectOnly base ctor (0x101fa0)
// retail calls first - exactly the shape the old per-TU views hand-rolled.
#include <Gruntz/SBI_Image.h>            // CSBI_RectOnly + CSBI_Image             (tag 3)
#include <Gruntz/SBI_ImageSet.h>         // CSBI_ImageSet                          (tag 4)
#include <Gruntz/SBI_ImageSetAni.h>      // CSBI_ImageSetAni + CSBI_StatzTabArrow  (8 / 5)
#include <Gruntz/SBI_WellGoo.h>          // CSBI_WellGoo                           (tag 7)
#include <Gruntz/SBI_WarlordHead.h>      // CSBI_WarlordHead                       (tag 0xb)
#include <Gruntz/SBI_GruntMachine.h>     // CSBI_GruntMachine                      (tag 9)
#include <Gruntz/SBI_StatzTabGruntBar.h> // CSBI_StatzTabGruntBar                  (tag 6)

// The icon/sprite factory the resource/game tabs pull chip + warpstone sprites
// from (g_gameReg.m_74 / m_68); __thiscall on the factory.
class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z); // thunk 0x4165 -> FUN_004e23c0
};
SIZE_UNKNOWN(CSbFactory);

// The m_cmdGrid probed here is a CTriggerMgr; Probe @0x79b30 is CTriggerMgr::ByteTableHas.
class CSbIconSet {};
SIZE_UNKNOWN(CSbIconSet);

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
