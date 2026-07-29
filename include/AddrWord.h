#ifndef GRUNTZ_ADDRWORD_H
#define GRUNTZ_ADDRWORD_H

#include <Ints.h>

// An address carried in a DWORD slot.
//
// Several engine tables declare their key/id column `int` and store a POINTER in it,
// and the binary shows BOTH readings of the same four bytes:
//
//   * CVariantSlot's record table - ?Find@CVariantSlot@@QAEHH@Z takes H and 0x16e1d0
//     probes it with a signed integer compare (`sub edx,ebx; jns`), while every caller
//     hands it the CALLER'S ADDRESS.
//   * the WWD object maps - CGameObject::m_188 is the i32 id counter, and CMapPtrToPtr
//     keys on `void*`, so the same word is the map key.
//   * g_buteTree - CMapStringToPtr values are `void*`, the stored value is a section id.
//   * CGruntSpawnConfig's five-argument voice driver - its source-id slot takes an
//     object id from CWarlord and a raw CGrunt* from the entrance path (0x63073
//     `push esi`), i.e. retail's own pointer-as-id.
//
// Naming both readings here keeps each of those slots honest without a pun at the site.
union AddrWord {
    void* m_addr;
    i32 m_word;
    u32 m_uword;
};

#endif // GRUNTZ_ADDRWORD_H
