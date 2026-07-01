// TriggerMgrEh.cpp - the /GX (eh) CTriggerMgr methods, split off the frameless triggermgr
// TU (C:\Proj\Gruntz). MSVC5's /GX frames any method that owns a destructible local (a
// CString error/Format temporary) or a `new`+ctor lifetime; these four cannot share the base
// TU's frameless flags without re-framing its matched leaves. The split is matching-neutral
// (each method is RVA-keyed); see docs/patterns/split-tu-eh-dtor-vs-frameless-cstring.md and
// the SBI_RectOnly / ChatBox precedents.
//
// LAYOUT NOTE: these methods touch `this` by raw offset (the opaque-shell convention of the
// whole class). Only the offsets + reloc-masked helpers each method touches are modelled.
#include <Gruntz/TriggerMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)

// Shared globals (same symbols as TriggerMgr.cpp; re-declared local to this TU).
struct CTmGameRegE {
    void ReportError(i32 code, i32 flags); // 0x8dc60 (reloc-masked)
    char p0[0x2c];
    char* m_2c; // +0x2c  the active world/play object
};
SIZE_UNKNOWN(CTmGameRegE);
extern CTmGameRegE* g_gameReg;
extern i32 g_644c54;
extern void* g_renderCtx; // ?g_renderCtx@@3PAXA @0x644ca4 (Load reads into it)

// CButeMgr (?g_buteMgr@@3VCButeMgr@@A @0x6453d8) - the canonical CButeMgr (via
// TriggerMgr.h); the int-with-default getter (0x171aa0) is reloc-masked.
extern CButeMgr g_buteMgr;

// A CString temporary as the error-Format path uses it (ctor/dtor + Format are the static MFC
// bodies, reloc-masked); the destructible temp forces the /GX frame.
struct CTmStr {
    CTmStr();                                   // 0x5b9b93
    ~CTmStr();                                  // 0x5b9cde
    void Format(const char* fmt, i32 a, i32 b); // 0x5b2cf5
    const char* c_str() const;                  // identity getter (inlined)
    char* m_buf;
};
SIZE_UNKNOWN(CTmStr);

// A logic/cell/list opaque shell whose reloc-masked __thiscall hooks the drivers dispatch.
struct CTmObj {
    inline CTmObj();
    inline void* operator new(u32);

    void* m_vt;                      // +0x00
    char _padToEnd[0x40 - 0x4];      // shell; real object is 0x40 B (the new(0x40) operand)
    i32 Apply();                     // vtbl +0x20
    void Run();                      // vtbl +0xc
    i32 Probe();                     // 0x6da... thiscall, no arg
    i32 Place(i32 a, i32 b, i32 c);  // thiscall placement
    void Reset();                    // thiscall
    void Ctor();                     // 0x49ce8 in-place ctor
    void Dtor();                     // 0x5b48c6 list dtor
    void Snap(i32* outR, i32* outC); // thiscall snap-to-cell
};
SIZE(CTmObj, 0x40); // measured: new(0x40) -> ctor 0x49ce8
void* operator new(u32);
void operator delete(void*);

inline CTmObj::CTmObj() {
    Ctor();
}

inline void* CTmObj::operator new(u32) {
    return ::operator new(0x40);
}

// The destroy-array CRT helper (reloc-masked @0x51f640) used by the destructor.
void Tm_DestroyArray(void* base, i32 stride, i32 count, void* dtor); // 0x11f640

// ---------------------------------------------------------------------------
// Load (0x7abc0) collaborators. The manager reads its state through the archive
// reader `ar` (vtable slot 0x2c = Read(dst, size)); the map values it resolves
// carry a type-id virtual (slot 8 = vtbl+0x20) and a +0x7c sub-object whose +0x18
// is the real placed game-object.
// ---------------------------------------------------------------------------
struct CTmSerReader {
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void Read(void* dst, i32 size); // slot 11 = vtbl+0x2c
};
SIZE_UNKNOWN(CTmSerReader);
struct CTmSerAux {
    char pad00[0x18];
    void* m_18; // +0x18  the placed game-object
};
SIZE_UNKNOWN(CTmSerAux);
struct CTmSerMapObj {
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual i32 GetTypeId(); // slot 8 = vtbl+0x20
    char pad04[0x7c - 0x4];
    CTmSerAux* m_7c; // +0x7c
};
SIZE_UNKNOWN(CTmSerMapObj);
// The level object (this->m_22c); its +0x8 host owns the name->object map at +0x48.
struct CTmSerMap {
    i32 Lookup(i32 key, void** out); // 0x1b8760 (__thiscall, ret 8)
};
SIZE_UNKNOWN(CTmSerMap);
// The manager's embedded list nodes (base list @this+0, record @+0x240, the ten
// selection lists @+0x2d0) and the +0x260 byte array; reloc-masked MFC bodies.
struct CTmSerList {
    void RemoveAll();      // 0x1b48a6
    void AddTail(void* p); // 0x1b4991
};
SIZE_UNKNOWN(CTmSerList);
struct CTmSerByteArray {
    void SetSize(i32 n, i32 grow); // 0x1b52e8
    void SetAtGrow(i32 i, i32 v);  // 0x1b5485
};
SIZE_UNKNOWN(CTmSerByteArray);
// The overlay sub-object (this->m_25c): new(0x40) + ctor, its own Load, and a
// Clear + custom free on teardown.
struct CTmSerOverlay {
    CTmSerOverlay();            // 0x9090 (ctor via new)
    void Clear();               // 0x92e0
    i32 Load(CTmSerReader* ar); // 0x9bb0
    inline void* operator new(u32);
    char m_body[0x40];
};
SIZE_UNKNOWN(CTmSerOverlay);
inline void* CTmSerOverlay::operator new(u32) {
    return ::operator new(0x40);
}
void Tm_RezFree(void* p); // 0x1b9b82 (__cdecl free used by the overlay teardown)

// 0x7abc0: Load(ar) - deserialize the whole trigger-mgr state (see the header). The
// grid + list loads resolve each stored key through the level's map, validating the
// found descriptor's type/sub-object; the overlay sub-object is rebuilt via new+Load.
// @early-stop
// /GX EH-state wall (same family as DestroyGroup / ApplySwitch in this TU): the
// full read/lookup/list-load body and the field offsets are faithful, but the
// overlay new-expression's partial-object cleanup states and the heavy stack-slot
// reuse (retail folds `this` and the lookup-out param into one slot) number/allocate
// differently than retail's __ehfuncinfo. topic:wall topic:eh.
RVA(0x0007abc0, 0x4b6)
i32 CTriggerMgr::Load(CTmSerReader* ar) {
    if (ar == 0) {
        return 0;
    }
    char* lvl = *(char**)((char*)this + 0x22c);
    if (lvl == 0) {
        return 0;
    }
    *(i32*)((char*)this + 0x3f0) = 0;
    *(i32*)((char*)this + 0x3f4) = 0;
    *(i32*)((char*)this + 0x3f8) = 0;
    *(i32*)((char*)this + 0x3fc) = 0;

    CTmSerMap* map = (CTmSerMap*)(*(char**)(lvl + 0x8) + 0x48);

    // the 4x15 placed-object grid (this[7..66], byte offsets +0x1c..+0x108)
    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, 4);
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = map->Lookup(key, &found) ? found : 0;
                if (looked == 0) {
                    return 0;
                }
                cell = ((CTmSerMapObj*)looked)->m_7c->m_18;
                if (cell == 0) {
                    return 0;
                }
            }
            ((void**)this)[base + i] = cell;
        }
    }

    // per-row state bands
    ar->Read((char*)this + 0x10c, 0x10);
    ar->Read((char*)this + 0x11c, 0xf0);
    ar->Read((char*)this + 0x20c, 0x10);
    ar->Read((char*)this + 0x21c, 0x10);

    // the +0x260 byte table
    i32 count;
    u32 ci;
    ar->Read(&count, 4);
    CTmSerByteArray* arr = (CTmSerByteArray*)((char*)this + 0x260);
    arr->SetSize(0, -1);
    for (ci = 0; ci < (u32)count; ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    // the +0x240 record list (nodes pulled off the shared free-list)
    ar->Read(&count, 4);
    CTmSerList* rec = (CTmSerList*)((char*)this + 0x240);
    for (ci = 0; ci < (u32)count; ci++) {
        char* fl = (char*)g_freeList;
        void* node = 0;
        if (*(void**)fl != 0) {
            node = fl + 4;
            g_freeList = *(void**)fl;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    // the ten selection lists (+0x2d0, stride 0x1c)
    char* sel = (char*)this + 0x2d0;
    i32 slot = 0xa;
    do {
        ar->Read(&count, 4);
        for (ci = 0; ci < (u32)count; ci++) {
            char* fl = (char*)g_freeList;
            void* node = 0;
            if (*(void**)fl != 0) {
                node = fl + 4;
                g_freeList = *(void**)fl;
            }
            ar->Read(node, 8);
            ((CTmSerList*)sel)->AddTail(node);
        }
        sel += 0x1c;
    } while (--slot != 0);

    // the type-5 singleton (+0x23c)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup(key, &found) ? found : 0;
            void* obj = (looked != 0 && ((CTmSerMapObj*)looked)->GetTypeId() == 5) ? looked : 0;
            *(void**)((char*)this + 0x23c) = obj;
            if (obj == 0) {
                return 0;
            }
        }
    }

    // the pending-fx singleton (+0x2a0)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup(key, &found) ? found : 0;
            if (looked == 0) {
                return 0;
            }
            void* obj = ((CTmSerMapObj*)looked)->m_7c->m_18;
            *(void**)((char*)this + 0x2a0) = obj;
            if (obj == 0) {
                return 0;
            }
        } else {
            *(void**)((char*)this + 0x2a0) = 0;
        }
    }

    // the base object list (this+0): reload from count keys
    ar->Read((char*)this + 0x274, 0x10);
    ((CTmSerList*)this)->RemoveAll();
    ar->Read(&count, 4);
    for (ci = 0; ci < (u32)count; ci++) {
        i32 key;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        void* found = 0;
        void* looked = map->Lookup(key, &found) ? found : 0;
        if (looked == 0) {
            return 0;
        }
        void* obj = ((CTmSerMapObj*)looked)->m_7c->m_18;
        if (obj == 0) {
            return 0;
        }
        ((CTmSerList*)this)->AddTail(obj);
    }

    // the overlay sub-object (+0x25c): tear down the old, rebuild + Load the new
    CTmSerOverlay* old = *(CTmSerOverlay**)((char*)this + 0x25c);
    if (old != 0) {
        old->Clear();
        Tm_RezFree(old);
        *(CTmSerOverlay**)((char*)this + 0x25c) = 0;
    }
    i32 hasOverlay;
    ar->Read(&hasOverlay, 4);
    if (hasOverlay != 0) {
        CTmSerOverlay* ov = new CTmSerOverlay;
        *(CTmSerOverlay**)((char*)this + 0x25c) = ov;
        if (ov->Load(ar) == 0) {
            return 0;
        }
    }

    // tail scalars + two globals
    ar->Read((char*)this + 0x230, 4);
    ar->Read((char*)this + 0x284, 4);
    ar->Read((char*)this + 0x288, 4);
    ar->Read((char*)this + 0x234, 8);
    ar->Read((char*)this + 0x2a4, 4);
    ar->Read((char*)this + 0x3ec, 4);
    ar->Read((char*)this + 0x400, 4);
    ar->Read(&g_644c54, 4);
    ar->Read(&g_renderCtx, 4);
    ar->Read((char*)this + 0x2a8, 4);
    ar->Read((char*)this + 0x3e8, 4);
    return 1;
}

// 0x6d300: ApplySwitch(sx, sy) - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
// sample the tile attribute, decode the logic class, switch over the kind dispatching the
// matching switch/trigger logic object's Apply; on a miss Format an error CString ("No switch
// logic found for switch at: x=%d, y=%d" / "No trigger logic ...") into a stack temp and
// ReportError. (__stdcall: ret 0xc.) Reconstructed to plateau.
// @early-stop
// big /GX switch driver (0x5b2 B): the dense jump table + the six CString-error stanzas
// (ctor/Format/ReportError/dtor under the EH frame) diverge wholesale in regalloc and the
// __ehfuncinfo state numbering; the validated head + the error-Format shape are faithful.
// topic:wall topic:eh.
RVA(0x0006d300, 0x5b2)
i32 CTriggerMgr::ApplySwitch(i32 sx, i32 sy) {
    char* plane = g_gameReg->m_2c;
    char* view = *(char**)(*(char**)((char*)this + 0x22c) + 0x24);
    i32 x = sx;
    i32 y = sy;
    if (x < 0) {
        x = 0;
    } else {
        i32 w = *(i32*)(*(char**)(view + 0x5c) + 0x30);
        if (x >= w) {
            x = w - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        i32 h = *(i32*)(*(char**)(view + 0x5c) + 0x34);
        if (y >= h) {
            y = h - 1;
        }
    }
    char* scroll = *(char**)(view + 0x5c);
    i32 sh = *(i32*)(scroll + 0x8c);
    i32 sw = *(i32*)(scroll + 0x90);
    i32 cell = *(i32*)(*(char**)(scroll + 0x24) + (y >> sw) * 4) + (x >> sh);
    i32 attr = *(i32*)(*(char**)(scroll + 0x20) + cell * 4);
    i32 kind;
    if (attr == (i32)0xeeeeeeee || attr == -1) {
        kind = 0;
    } else {
        CTmObj* logic = (CTmObj*)*(void**)(*(char**)(view + 0x4c) + (attr & 0xffff) * 4);
        kind = logic->Apply();
    }
    i32 op = kind - 0x34;
    if ((u32)op > 0xe) {
        return 0;
    }
    i32 cx = x;
    i32 cy = y;
    CTmObj* obj = (CTmObj*)*(void**)(*(char**)(plane + 0x2e4) + 0);
    if (obj == 0) {
        CTmStr msg;
        msg.Format("No switch logic found for switch at: x=%d, y=%d", cx >> 5, cy >> 5);
        g_gameReg->ReportError(0x80dd, 0x3f7);
        return 0;
    }
    obj->Run();
    return 1;
}

// 0x798d0: DestroyGroup(col, row, force) - lazily create the overlay sub-object (+0x25c) via
// new+ctor (the /GX frame guards the partially-constructed object); if it fails to take, tear
// it back down and ReportError(0x800a). When it already exists, route by the magic group to
// the place helper. ret 1 on placement. (__stdcall: ret 0x10.) Reconstructed to plateau.
// @early-stop
// /GX new+ctor wall: the placement-new lifetime + the teardown-on-failure path carry the EH
// frame whose state numbering + partial-object cleanup diverge from retail; the alloc/ctor/
// teardown shape is faithful. topic:wall topic:eh.
RVA(0x000798d0, 0x1b6)
i32 CTriggerMgr::DestroyGroup(i32 col, i32 row, i32 force) {
    (void)force;
    CTmObj* ov = *(CTmObj**)((char*)this + 0x25c);
    if (ov == 0) {
        CTmObj* fresh = new CTmObj;
        *(CTmObj**)((char*)this + 0x25c) = fresh;
        if (((CTmObj*)this)->Probe() == 0) {
            CTmObj* o2 = *(CTmObj**)((char*)this + 0x25c);
            if (o2 != 0) {
                o2->Dtor();
                operator delete(o2);
                *(CTmObj**)((char*)this + 0x25c) = 0;
            }
            g_gameReg->ReportError(0x800a, 0x3ff);
        }
        return 0;
    }
    if (*(i32*)((char*)ov + 0x2c) != 0 || *(i32*)((char*)this + 0x24c) != 1) {
        return 0;
    }
    i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
    char* cellp = *(char**)((char*)this + (rec[1] + rec[0] * 15) * 4 + 0x1c);
    if (cellp == 0 || *(i32*)(cellp + 0x1ec) != g_644c54) {
        return 0;
    }
    if (((CTmObj*)this)->Place(*(i32*)(cellp + 0x1f0), *(i32*)(cellp + 0x1ec), 0) == 0) {
        return 0;
    }
    char* view = *(char**)(*(char**)((char*)this + 0x22c) + 0x24);
    char* sc = *(char**)(view + 0x5c) + 0x40;
    i32 ox = *(i32*)(sc) - *(i32*)(view + 0x14) + row;
    i32 oy = *(i32*)(sc + 0x4) - *(i32*)(view + 0x10) + col;
    ((CTmObj*)this)->Place(oy, ox, 1);
    return 1;
}

// 0x79b80: ReinitGroup(col, row) - when not already done (+0x284) and the level is active,
// Format a "Level%i" CString from the level index, read the WarpStone config color, hit-test
// the (col,row) target, lazily re-init the status-bar item (+0x2dc) and either flag it done or
// recycle the record node; mark +0x284 done. (__stdcall: ret 0x8.) Reconstructed to plateau.
// @early-stop
// /GX CString-temp wall: the Level%i Format temporary forces the EH frame whose state +
// cleanup diverge; the Format/GetColor/hit-test/status path is faithful. topic:wall topic:eh.
RVA(0x00079b80, 0x194)
i32 CTriggerMgr::ReinitGroup(i32 col, i32 row) {
    if (*(i32*)((char*)this + 0x284) != 0) {
        return 0;
    }
    if (*(i32*)((char*)g_gameReg + 0x134) != 1) {
        return 0;
    }
    char* lvl = g_gameReg->m_2c;
    CTmStr name;
    name.Format("Level%i", *(i32*)(lvl + 0x1c), 0);
    i32 color = g_buteMgr.GetIntDef((char*)name.c_str(), "WarpStone", 0);
    i32 hx = col;
    i32 hy = row;
    if (hy >= *(i32*)((char*)g_gameReg + 0x144) || hy < *(i32*)((char*)g_gameReg + 0x13c)
        || hx >= *(i32*)((char*)g_gameReg + 0x148) || hx < *(i32*)((char*)g_gameReg + 0x140)) {
        ((CTmObj*)lvl)->Place(hy, hx, 0);
    }
    char* plane = *(char**)(*(char**)((char*)g_gameReg + 0x30) + 0x24);
    i32 outR = col;
    i32 outC = row;
    ((CTmObj*)plane)->Snap(&outR, &outC);
    char* sbi = *(char**)((char*)lvl + 0x2dc);
    if (*(i32*)(sbi + 0x548) == 0) {
        if (*(i32*)sbi == 2) {
            ((CTmObj*)sbi)->Reset();
        }
        if (*(i32*)(sbi + 0x10c) != 5) {
            ((CTmObj*)sbi)->Place(5, 3, 0);
        }
        ((CTmObj*)sbi)->Place(5, 1, 0);
        ((CTmObj*)sbi)->Run();
    }
    if (((CTmObj*)(*(char**)((char*)lvl + 0x2dc)))->Place(color, outR, outC) != 0) {
        *(i32*)(*(char**)((char*)lvl + 0x2dc) + 0x548) = 1;
    } else {
        ((CTmObj*)((char*)this + 0x260))->Place(*(i32*)((char*)this + 0x268), 0, 0);
    }
    *(i32*)((char*)this + 0x284) = 1;
    return 1;
}

// 0x85c50: ~CTriggerMgr - the /GX destructor: Cleanup (drain the lists), then destruct the 10
// selection lists (+0x2d0, stride 0x1c, EH state 2), the +0x260 list (state 1), the +0x240
// list (state 0) and the embedded base list (state -1). (__thiscall, ret.) Reconstructed to
// plateau.
// @early-stop
// /GX member-array dtor wall: the destroy-array helper + the four staged member-dtor EH
// states number differently than retail's __ehfuncinfo; the teardown sequence is faithful.
// topic:wall topic:eh.
RVA(0x00085c50, 0x83)
CTriggerMgr::~CTriggerMgr() {
    Cleanup();
    Tm_DestroyArray((char*)this + 0x2d0, 0x1c, 0xa, 0);
    ((CTmObj*)((char*)this + 0x260))->Dtor();
    ((CTmObj*)((char*)this + 0x240))->Dtor();
    ((CTmObj*)this)->Dtor();
}
