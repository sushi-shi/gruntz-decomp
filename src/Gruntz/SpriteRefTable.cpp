// SpriteRefTable.cpp - the game-registry sprite/animation reference table (trace
// placeholder ClassUnknown_10). Methods in ascending retail-RVA order. The table
// shape (two 17-slot CSpriteRef buckets at +0x08 / +0x4c) and the CSpriteRef /
// CSpriteRefHashTable helpers come from <Gruntz/SpriteRefTable.h>; every cross-
// cluster callee (the node ctor/dtor, the hash Lookup, the engine alloc/free) is
// modeled NO-body so its `call` reloc-masks.
#include <Win32.h>

#include <Gruntz/SpriteRefTable.h>

#include <rva.h>

// Engine CRT/resource helpers reached by Add()/Clear(); reloc-masked DIR32/REL32.
extern "C" void* RezAlloc(u32 n); // 0x1b9b46 (operator new / RezAlloc)
extern "C" void RezFree(void* p); // 0x1b9b82

// m_04->m_18 is the sprite manager; +0x10 of it is the name->sprite hash table.
struct CSpriteMgrHolder {
    char m_pad00[0x18];
    char* m_18; // +0x18  the sprite mgr (its +0x10 is the CSpriteRefHashTable)
};

// The object Lookup writes into `out`: +0x10 is the sprite, whose +0xc holds the
// frame data fed to the alpha factory.
struct CLookupSprite {
    char m_pad00[0xc];
    void* m_0c; // +0x0c  frame data
};
struct CLookupResult {
    char m_pad00[0x10];
    CLookupSprite* m_10; // +0x10
};

// ---------------------------------------------------------------------------

RVA(0x000e2250, 0x26)
i32 CSpriteRefTable::Init(i32 p0, i32 p1) {
    if (!p0) {
        return p0;
    }
    m_00 = (CSpriteRefFactory*)p0;
    m_04 = (void*)p1;
    m_90 = 0;
    return 1;
}

RVA(0x000e2290, 0x2a)
void CSpriteRefTable::Reset() {
    Clear();
    m_00 = 0;
    m_04 = 0;
    m_90 = 0;
    for (i32 i = 0; i < 0x11; i++) {
        m_refA[i] = 0;
        m_refB[i] = 0;
    }
}

RVA(0x000e22d0, 0x6e)
void CSpriteRefTable::Clear() {
    if (m_00) {
        for (i32 i = 0; i < 0x11; i++) {
            CSpriteRef* a = GetA(i);
            if (a) {
                a->Free();
                RezFree(a);
            }
            CSpriteRef* b = GetB(i);
            if (b) {
                b->Free();
                RezFree(b);
            }
        }
        for (i32 j = 0; j < 0x11; j++) {
            m_refA[j] = 0;
            m_refB[j] = 0;
        }
        m_90 = 0;
    }
}

RVA(0x000e2360, 0x15)
CSpriteRef* CSpriteRefTable::GetA(i32 i) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    return m_refA[i];
}

RVA(0x000e2390, 0x15)
CSpriteRef* CSpriteRefTable::GetB(i32 i) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    return m_refB[i];
}

RVA(0x000e23c0, 0x2d)
i32 CSpriteRefTable::GetSel(i32 i, i32 bAlt) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    CSpriteRef* node = bAlt ? m_refB[i] : m_refA[i];
    if (!node) {
        return 0;
    }
    return node->m_04;
}

// @early-stop
// out-param zero-init scheduling wall: retail SINKS `mov [&out],0` past both arg
// pushes, cl HOISTS it between them (identical multiset, 1 instr permuted); the
// rest of the 182-byte body is byte-exact. docs/patterns/outparam-zeroinit-scheduling.md
RVA(0x000e2890, 0xb6)
CSpriteRef* CSpriteRefTable::Add(char* szName, i32 kind) {
    void* out = 0;
    CSpriteRefHashTable* tbl = (CSpriteRefHashTable*)(((CSpriteMgrHolder*)m_04)->m_18 + 0x10);
    tbl->Lookup(szName, &out);
    if (!out) {
        return 0;
    }
    void* sprite = ((CLookupResult*)out)->m_10->m_0c;
    if (!sprite) {
        return 0;
    }
    void* alpha = m_00->AlphaTable(sprite);
    if (!alpha) {
        return 0;
    }
    CSpriteRef* node;
    CSpriteRef* tmp = (CSpriteRef*)RezAlloc(0x10);
    if (tmp) {
        tmp->m_00 = 0;
        tmp->m_04 = 0;
        node = tmp;
    } else {
        node = 0;
    }
    if (node->Build((i32)m_00, alpha, kind) == 0) {
        if (node) {
            node->Free();
            RezFree(node);
        }
        return 0;
    }
    return node;
}
