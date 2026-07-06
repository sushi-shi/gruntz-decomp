#include <rva.h>
#include <Bute/SymTab.h>
#include <Mfc.h> // CString, CObList, CMapStringToOb/Ptr, POSITION (reloc-masked engine MFC)
#include <Gruntz/AreaMgr.h> // CAreaMgr (this) + the canonical CSpawnList/CSpawnEntry
#include <stdio.h>          // sprintf (reloc-masked engine CRT)
#include <string.h>         // strncmp (reloc-masked engine CRT _strncmp)

// LoadObject{Image,Sound,Anim}Resources (0x9a510/0x9a910/0x9ac20) - the three
// near-identical per-spawn-entry asset reconcilers for the OBJECTZ_ namespace
// tree.  __thiscall(this=tree, SpawnEntry* entry, ResRegistry* src):
//   1. ClearFlags() resets every existing child entry's +4 "wanted" flag to 0.
//   2. Walk the source registry's CMapString map (embedded at entry->reg+0x10),
//      and for each "OBJECTZ_"-prefixed key: FindAdd it in this tree; if it already
//      exists mark its flag, else queue the source value in a local CObList of new
//      objects.
//   3. Drain that CObList, handing each new source object to the registry's
//      ProcessNew (Image: a vtable slot; Sound/Anim: a direct method).
//   4. Re-scan the child entries; for each still-unwanted (flag==0) one, build the
//      "{IMAGEZ|SOUNDZ|ANIZ}_<name>" key, ResolvePath it in src, and Install it
//      through the registry, then mark the flag.  The Image arm brackets the
//      install with the g_resourceInstallActive tile-counter gate.
// The local CObList carries a destructor -> the /GX exception frame.  Only offsets
// / code bytes are load-bearing; helpers are reloc-masked engine externs.

DATA(0x002bf37c)
extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA (Image install bracket)

// (The old CObjResNode / CObjResListNode / CObjResBuilder / CObjResTree views
// were folded onto the canonicals: the "child entry" is CSpawnEntry (GetSpriteName
// == GetTail @0x9a830, GetAssetName == GetName @0x9a260, the +4 wanted flag ==
// m_flag), the "builder" is CSpawnList (FindAdd == FindByName @0x9a290, ClearFlags
// @0x9a420, head/cursor == m_list/m_cursor), and the "tree" is CAreaMgr itself
// (the builder at +0x04 is m_spawnEntryList; same retail TU band). See
// <Gruntz/SpawnList.h> for the unification proof.)

// The Image registry (entry->m_10). This is a FOREIGN engine class: its ??_7 and
// the intermediate slots are unreconstructed engine code, so the honest model names
// only the TWO dispatched slots - Install at vtable slot 18 (+0x48) and ProcessNew
// at slot 20 (+0x50), both __thiscall. Its CMapStringToOb source map is embedded at
// +0x10. The slots are modeled as 4-byte member-function pointers loaded from the
// vtable (`char m_pad` runs document the un-recovered slots) so the calls emit
// `mov eax,[ecx]; call [eax+0xNN]`. Class COMPLETE before the T::* typedefs so each
// PMF stays 4 bytes (docs/patterns/pmf-complete-class-4byte.md).
struct ObjImageRegistryVtbl;
struct ObjImageRegistry {
    ObjImageRegistryVtbl* m_vtbl;                     // +0x00
    void Install(void* h, char* name, const char* g); // slot 18 (+0x48) via vtbl
    void ProcessNew(CObject* val);                    // slot 20 (+0x50) via vtbl
    char m_pad04[0x10 - 0x4];
    CMapStringToOb m_map; // +0x10  source map (CString -> CImage*)
};
typedef void (ObjImageRegistry::*ObjImgInstallFn)(void* h, char* name, const char* g);
typedef void (ObjImageRegistry::*ObjImgProcessFn)(CObject* val);
SIZE_UNKNOWN(ObjImageRegistryVtbl);
struct ObjImageRegistryVtbl {
    char m_pad00[0x48];
    ObjImgInstallFn Install; // +0x48
    char m_pad4c[0x50 - 0x4c];
    ObjImgProcessFn ProcessNew; // +0x50
};
inline void ObjImageRegistry::Install(void* h, char* name, const char* g) {
    (this->*(m_vtbl->Install))(h, name, g);
}
inline void ObjImageRegistry::ProcessNew(CObject* val) {
    (this->*(m_vtbl->ProcessNew))(val);
}

// The Sound registry (entry->m_28): concrete; CMapStringToPtr source map at +0x10,
// ProcessNew/Install are direct __thiscall methods.
struct ObjSoundRegistry {
    void ProcessNew(void* val);                       // 0x157b00 __thiscall
    void Install(void* h, char* name, const char* g); // 0x157ee0 __thiscall
    char m_pad04[0x10];
    CMapStringToPtr m_map; // +0x10
};

// The Anim registry (entry->m_2c): concrete; same shape as Sound, different methods.
struct ObjAnimRegistry {
    void ProcessNew(void* val);                       // 0x152660 __thiscall
    void Install(void* h, char* name, const char* g); // 0x152ad0 __thiscall
    char m_pad04[0x10];
    CMapStringToPtr m_map; // +0x10
};

// The per-spawn entry (arg1): the three registry slots.
struct ObjSpawnEntry {
    char m_pad00[0x10];
    ObjImageRegistry* m_10; // +0x10
    char m_pad14[0x28 - 0x14];
    ObjSoundRegistry* m_28; // +0x28
    ObjAnimRegistry* m_2c;  // +0x2c
};

// The resolution source (arg2): ResolvePath looks a namespaced key up, returning a
// handle (0 if absent).

// @source: decomp-xref
// @early-stop
// ~88.7%: complete + correct (the OBJECTZ_ GetNextAssoc scan, FindAdd reconcile,
// CObList drain via the registry's ProcessNew, the IMAGEZ_%s sprintf + CSymTab
// ResolvePath + the polymorphic vtable Install, and the g_resourceInstallActive bracket
// are all byte-faithful, strings/relocs aligned). Residual: retail's frame is 0xb4
// vs this build's 0xac because retail reserves a guarded CString cleanup slot
// ([esp+0x24] with its construction guard at [esp+0x1c]) that it NEVER constructs
// nor sets in the normal path - dead EH-cleanup scaffolding the optimizer emitted
// for an elided by-value name temp - plus the matching conditional ~CString block.
// That phantom +8 shifts every [esp+N] operand, capping the fuzzy score though the
// instruction selection is identical. Not source-steerable without reproducing the
// elided-but-scaffolded temp. Reused for the Sound/Anim siblings below.
RVA(0x0009a510, 0x275)
i32 CAreaMgr::LoadObjectImageResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* srcMap = &entry->m_10->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CObject* obj = toAdd.GetNext(dp);
        entry->m_10->ProcessNew(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            g_resourceInstallActive = 1;
            sprintf(buf, "IMAGEZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_10->Install(handle, (char*)(LPCTSTR)e->GetName(), "");
            g_resourceInstallActive = 0;
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Image sibling (SOUNDZ_ key, CMapStringToPtr source,
// the concrete entry->m_28 ProcessNew/Install, no install-gate bracket). Same phantom
// guarded-CString +8 frame-shift wall documented on the Image arm above.
RVA(0x0009a910, 0x261)
i32 CAreaMgr::LoadObjectSoundResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_28->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail((CObject*)val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CObject* obj = toAdd.GetNext(dp);
        entry->m_28->ProcessNew(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "SOUNDZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_28->Install(handle, (char*)(LPCTSTR)e->GetName(), "");
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Sound sibling (ANIZ_ key, the concrete entry->m_2c
// ProcessNew/Install). Same phantom guarded-CString +8 frame-shift wall as above.
RVA(0x0009ac20, 0x261)
i32 CAreaMgr::LoadObjectAnimResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_2c->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail((CObject*)val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CObject* obj = toAdd.GetNext(dp);
        entry->m_2c->ProcessNew(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "ANIZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_2c->Install(handle, (char*)(LPCTSTR)e->GetName(), "");
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
}

SIZE_UNKNOWN(ObjAnimRegistry);
SIZE_UNKNOWN(ObjImageRegistry);
SIZE_UNKNOWN(ObjSoundRegistry);
SIZE_UNKNOWN(ObjSpawnEntry);
