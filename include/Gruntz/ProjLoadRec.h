// ProjLoadRec.h - CProjLoadRec, the CProjectile/CTimeBomb-family dual-mode record
// loader (Load @0x0e0d40).
//
// @identity-recovered / FOLD DEFERRED: CProjLoadRec IS CProjectile, and Load @0x0e0d40
// IS CProjectile::SerializeMove (vtable slot 1). PROOF: vtable_hierarchy --class
// CProjectile resolves slot 1 (ILT thunk 0x0034b3) to this RVA under "Load"; the field
// layout mirrors CProjectile exactly (m_1e0[7] = the seven frame sprites, m_1fc =
// m_shadow, m_204 = m_hitList, m_220/m_224 = m_targetId/m_ownerId, and the +0x150
// sub-record restores the same owner/sprite/m_7c triple the ctor sets). The full fold is
// a whole-TU re-home: 0x0e0d40 lives in Projectile.cpp's RVA band (0xdec60..0xe2213), so
// folding onto CProjectile means moving this @early-stop body into Projectile.cpp AND
// reconciling m_1e0[]/m_204 with CProjectile's named-field/CPtrList layout - left for the
// projectile-serialize rehome. Defined in src/Gruntz/ProjLoadRec.cpp.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_PROJLOADREC_H
#define GRUNTZ_PROJLOADREC_H

#include <rva.h>
#include <Ints.h>
#include <Rez/RezList.h>           // CRezList (the +0x204 value member) + CRezListNode
#include <Gruntz/SerialArchive.h> // CSerialArchive typedef (NEVER fwd-declare it as a class)

struct CGameObject;
struct AnimWorkerObj;
struct CObject; // MFC ::CObject (unified struct tag)
struct CoordNode;

struct CProjLoadRec {
    i32 Load(CSerialArchive* s, i32 mode, i32 a2, CGameObject* a3);      // 0x0e0d40
    i32 ChainLoad(CSerialArchive* s, i32 mode, i32 a2, CGameObject* a3); // 0x16f4a0

    char _00[0x150];
    CGameObject* m_150;                    // +0x150  a3
    CGameObject* m_154;                    // +0x154  a3
    AnimWorkerObj* m_158;                  // +0x158  a3->m_7c
    CObject* m_15c;                        // +0x15c  resolved value (CMapStringToPtr entry)
    i32 m_160, m_164, m_168, m_16c;        // +0x160  the 0x10-byte blob
    i32 m_170, m_174, m_178, m_17c, m_180; // +0x170
    i32 _184;
    i32 m_188, m_18c; // +0x188 (8)
    i32 m_190;        // +0x190
    i32 _194;
    i32 m_198, m_19c;               // +0x198 (8)
    i32 m_1a0, m_1a4;               // +0x1a0 (8)
    i32 m_1a8, m_1ac;               // +0x1a8 (8)
    i32 m_1b0, m_1b4;               // +0x1b0 (8)
    i32 m_1b8, m_1bc;               // +0x1b8 (8)
    i32 m_1c0, m_1c4;               // +0x1c0 (8)
    i32 m_1c8, m_1cc;               // +0x1c8 (8)
    i32 m_1d0, m_1d4, m_1d8, m_1dc; // +0x1d0
    CObject* m_1e0[7];              // +0x1e0..+0x1f8  name refs (CMapStringToPtr entries)
    CGameObject* m_1fc;             // +0x1fc  type-5 latch (the resolved game object)
    i32 m_200;                      // +0x200
    CRezList m_204;                 // +0x204  AddTail target
    CoordNode* m_208;               // +0x208  write-path node list
    i32 _20c;
    i32 m_210; // +0x210
    i32 _214, _218, _21c;
    i32 m_220, m_224; // +0x220, +0x224
};
SIZE_UNKNOWN();

#endif // GRUNTZ_PROJLOADREC_H
