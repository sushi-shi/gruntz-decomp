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

// Shared globals (same symbols as TriggerMgr.cpp; re-declared local to this TU).
struct CTmGameRegE {
    void ReportError(i32 code, i32 flags); // 0x8dc60 (reloc-masked)
    char p0[0x2c];
    char* m_2c; // +0x2c  the active world/play object
};
extern CTmGameRegE* g_gameReg;
extern i32 g_644c54;

// CButeMgr (?g_buteMgr@@3VCButeMgr@@A @0x6453d8) - GetColor reloc-masked.
struct CTmButeMgrE {
    i32 GetColor(const char* section, const char* key, i32 def); // 0x171aa0
};
extern CTmButeMgrE g_buteMgr;

// A CString temporary as the error-Format path uses it (ctor/dtor + Format are the static MFC
// bodies, reloc-masked); the destructible temp forces the /GX frame.
struct CTmStr {
    CTmStr();                                   // 0x5b9b93
    ~CTmStr();                                  // 0x5b9cde
    void Format(const char* fmt, i32 a, i32 b); // 0x5b2cf5
    const char* c_str() const;                  // identity getter (inlined)
    char* m_buf;
};

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
    i32* rec = (i32*)(*(char**)((char*)this + 0x244) + 0x8);
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
    i32 color = g_buteMgr.GetColor(name.c_str(), "WarpStone", 0);
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
