#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeTree.h>
#include <Bute/PTreeNode.h>
#include <Gruntz/TypeCollRuntime.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>

// The two container globals the whole Bute/act layer runs on.  They live in their
// own compiland because retail's `$E` dynamic-init helpers for them carry the
// constructor and destructor bodies INLINE (0x16e6a0 / 0x16e6e0 / 0x16e730 /
// 0x16e7a0), and MSVC 5 only inlines a ctor/dtor into a `$E` helper when the TU is
// compiled WITHOUT /GX - every function in TypeKeyColl.cpp's own compiland carries
// an EH frame, so that one is /GX and cannot be the source of these four bodies.
// See docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md.

inline CTypeCollRuntime::CTypeCollRuntime()

    : _zdvec(sizeof(CString), 0x7d0, 0x7da, ZVecNoScratch()) {
    CString* slot = Slots();
    if (slot != NULL) {
        i32 cnt = m_grown;
        while (cnt-- != 0) {
            if (slot != NULL) {
                slot->CString::CString();
            }
            ++slot;
        }
    }
}

inline CTypeCollRuntime::~CTypeCollRuntime() {
    CString* item = Elem(m_lo);
    if (item != NULL) {
        i32 count = m_hi - m_lo + 1;
        while (count-- != 0) {
            item->CString::~CString();
            ++item;
        }
    }
}

DATA(0x002bf650)
CTypeCollRuntime g_typeColl;

DATA(0x002bf620)
CButeTree g_buteTree(&ButeTreeNopFree, 0);

RVA_COMPGEN(0x0016e9c0, 0x45, ??_GCButeTree@@UAEPAXI@Z)
RVA_COMPGEN(0x0016ea20, 0x51, ??_GCTypeCollRuntime@@UAEPAXI@Z)
