#include <rva.h>
#include <Mfc.h>    // CString, CObList, CMapStringToOb/Ptr, POSITION (reloc-masked engine MFC)
#include <stdio.h>  // sprintf (reloc-masked engine CRT)
#include <string.h> // strncmp (reloc-masked engine CRT _strncmp)

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

// A child entry of the tree (the value stored in a builder node and returned by
// FindAdd).  Carries the +4 "wanted" flag and its asset name (returned by value).
struct CObjResNode {
    char m_pad00[0x4];
    i32 m_4;                 // +0x04  wanted flag (0 => needs (re)loading)
    CString GetSpriteName(); // 0x9a830 __thiscall, by-value (sprintf source name)
    CString GetAssetName();  // 0x9a260 __thiscall, by-value (Install source name)
};

// The builder's internal singly-linked list node (CNode-shaped): next at +0, the
// owned child entry at +8.
struct CObjResListNode {
    CObjResListNode* m_next; // +0x00
    char m_pad04[0x8 - 0x4];
    CObjResNode* m_obj; // +0x08
};

// The builder collection embedded at tree+4: a name-keyed set of child entries.
struct CObjResBuilder {
    char m_pad00[0x4];
    CObjResListNode* m_head; // +0x04  scan-list head
    char m_pad08[0x1c - 0x8];
    CObjResListNode* m_cursor;                // +0x1c  scan cursor (stored in the object)
    void ClearFlags();                        // 0x9a420 __thiscall
    CObjResNode* FindAdd(const CString& key); // 0x9a290 __thiscall, ret child-or-null
};

// The Image registry (entry->m_10): polymorphic; its CMapStringToOb source map is
// embedded at +0x10, Install is vtable slot 18 (+0x48), ProcessNew is slot 20 (+0x50).
struct ObjImageRegistry {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void Install(void* h, char* name, const char* g); // slot 18 (+0x48)
    virtual void v19();
    virtual void ProcessNew(CObject* val); // slot 20 (+0x50)
    char m_pad04[0x10 - 0x4];
    CMapStringToOb m_map; // +0x10  source map (CString -> CImage*)
};

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
struct ObjResLookup {
    void* ResolvePath(char* key); // 0x13bae0 __thiscall
};

// The tree node `this`: the builder collection is its base subobject at +4.
struct CObjResTree {
    char m_pad00[0x4];
    CObjResBuilder m_builder; // +0x04

    i32 LoadObjectImageResources(ObjSpawnEntry* entry, ObjResLookup* src);
    i32 LoadObjectSoundResources(ObjSpawnEntry* entry, ObjResLookup* src);
    i32 LoadObjectAnimResources(ObjSpawnEntry* entry, ObjResLookup* src);
};

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
i32 CObjResTree::LoadObjectImageResources(ObjSpawnEntry* entry, ObjResLookup* src) {
    if (entry == 0) {
        return 0;
    }
    m_builder.ClearFlags();

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
            CObjResNode* found = m_builder.FindAdd(key);
            if (found != 0) {
                found->m_4 = 1;
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

    CObjResBuilder* b = &m_builder;
    b->m_cursor = b->m_head;
    CObjResNode* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CObjResListNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_obj;
    }
    while (e != 0) {
        if (e->m_4 == 0) {
            char buf[0x80];
            g_resourceInstallActive = 1;
            sprintf(buf, "IMAGEZ_%s", (const char*)(LPCTSTR)e->GetSpriteName());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_10->Install(handle, (char*)(LPCTSTR)e->GetAssetName(), "");
            g_resourceInstallActive = 0;
            e->m_4 = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CObjResListNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_obj;
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
i32 CObjResTree::LoadObjectSoundResources(ObjSpawnEntry* entry, ObjResLookup* src) {
    if (entry == 0) {
        return 0;
    }
    m_builder.ClearFlags();

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
            CObjResNode* found = m_builder.FindAdd(key);
            if (found != 0) {
                found->m_4 = 1;
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

    CObjResBuilder* b = &m_builder;
    b->m_cursor = b->m_head;
    CObjResNode* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CObjResListNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_obj;
    }
    while (e != 0) {
        if (e->m_4 == 0) {
            char buf[0x80];
            sprintf(buf, "SOUNDZ_%s", (const char*)(LPCTSTR)e->GetSpriteName());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_28->Install(handle, (char*)(LPCTSTR)e->GetAssetName(), "");
            e->m_4 = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CObjResListNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_obj;
        }
    }
    return 1;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Sound sibling (ANIZ_ key, the concrete entry->m_2c
// ProcessNew/Install). Same phantom guarded-CString +8 frame-shift wall as above.
RVA(0x0009ac20, 0x261)
i32 CObjResTree::LoadObjectAnimResources(ObjSpawnEntry* entry, ObjResLookup* src) {
    if (entry == 0) {
        return 0;
    }
    m_builder.ClearFlags();

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
            CObjResNode* found = m_builder.FindAdd(key);
            if (found != 0) {
                found->m_4 = 1;
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

    CObjResBuilder* b = &m_builder;
    b->m_cursor = b->m_head;
    CObjResNode* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CObjResListNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_obj;
    }
    while (e != 0) {
        if (e->m_4 == 0) {
            char buf[0x80];
            sprintf(buf, "ANIZ_%s", (const char*)(LPCTSTR)e->GetSpriteName());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_2c->Install(handle, (char*)(LPCTSTR)e->GetAssetName(), "");
            e->m_4 = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CObjResListNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_obj;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CObjResBuilder);
SIZE_UNKNOWN(CObjResListNode);
SIZE_UNKNOWN(CObjResNode);
SIZE_UNKNOWN(CObjResTree);
SIZE_UNKNOWN(ObjAnimRegistry);
SIZE_UNKNOWN(ObjImageRegistry);
SIZE_UNKNOWN(ObjResLookup);
SIZE_UNKNOWN(ObjSoundRegistry);
SIZE_UNKNOWN(ObjSpawnEntry);
