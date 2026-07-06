// BoundaryUpper.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// reconstructed. These sit at class boundaries in GRUNTZ.EXE across the DinMgr2 /
// Dsndmgr / DDrawMgr / Rez engine modules; RTTI cannot attribute the COMDAT-folded
// leaf methods so the owning class names here are placeholders. Only OFFSETS + the
// code shape are load-bearing (campaign doctrine). Non-EH (base /O2) bodies only;
// the /GX EH-frame siblings live in BoundaryUpperEh.cpp. The per-use owner/referent
// views now live in <Gruntz/BoundaryUpperViews.h> (pure code motion).
#include <rva.h>
#include <Mfc.h> // superset of Win32.h: WINAPI + MFC CObArray (the +0x1dc array RemoveAll)
#include <Gruntz/BoundaryUpperViews.h> // owner/referent views for this TU (pulls Blk6c.h)

// The engine __cdecl deallocator (operator delete; reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p);

// The wap-object teardown grand-base vtable (0x5e8cb4); stamped by address.

// ---------------------------------------------------------------------------
// Embedded base-subobject vptr restamp (member dtor of the grand-base): the
// 7-byte `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf
// classes share the identical shape (each called from its own scalar-deleting
// dtor - e.g. 0x161460 <- ??_G @0x161440, confirmed by xref).
// TERMINAL manual stamps (not convertible to `: public Wap::CObject`): retail
// keeps these as THREE separate un-COMDAT-folded copies at 0x161460/0x161560/
// 0x163a10; letting cl emit ~Wap::CObject would fold them to ONE COMDAT and
// drop two of the three RVA pins. The reloc-masked stamp is already byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00161460, 0x7)
SW_161460::~SW_161460() {
} // cl auto-stamps the Wap::CObject vptr; retail's 7-byte manual stamp dropped (% ok)
RVA(0x00161560, 0x7)
SW_161560::~SW_161560() {
} // cl auto-stamps the Wap::CObject vptr; retail's 7-byte manual stamp dropped (% ok)
RVA(0x00163a10, 0x7)
SW_163a10::~SW_163a10() {
} // cl auto-stamps the Wap::CObject vptr; retail's 7-byte manual stamp dropped (% ok)

// ---------------------------------------------------------------------------
// 0x1413c0 - `return m_20 * n;` (CDirectDrawMgr-area scale helper). __thiscall,
// one stack arg.
// ---------------------------------------------------------------------------
RVA(0x001413c0, 0xb)
i32 B_1413c0::Scale(i32 n) {
    return m_20 * n;
}

// ---------------------------------------------------------------------------
// 0x1614b0 - `if(m_14) RezFree(m_14); m_14 = 0;` (CImageSet1-area buffer release).
// ---------------------------------------------------------------------------
RVA(0x001614b0, 0x1c)
void B_1614b0::Release() {
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
}

// ---------------------------------------------------------------------------
// 0x137300 - SoundDevice getter: `if(!m_78) return 0; if(!Probe()) return 0;
// return m_84;`. Probe (0x137260) is a __thiscall member, modeled no-body
// (reloc-masked rel32).
// ---------------------------------------------------------------------------
RVA(0x00137300, 0x23)
i32 SoundDevice::Get() {
    if (!m_78) {
        return 0;
    }
    if (!Probe()) {
        return 0;
    }
    return m_84;
}

// ---------------------------------------------------------------------------
// 0x1433d0 - CDdObArray ordered compare: unsigned m_c then m_8, then m_54 as a
// 0/1 result. __stdcall (callee-cleanup, 2 ptr args), no `this`.
// ---------------------------------------------------------------------------
RVA(0x001433d0, 0x4f)
i32 __stdcall Compare_1433d0(DdOb_1433d0* a, DdOb_1433d0* b) {
    if (a->m_c > b->m_c) {
        return 1;
    }
    if (a->m_c < b->m_c) {
        return 0;
    }
    if (a->m_8 > b->m_8) {
        return 1;
    }
    if (a->m_8 < b->m_8) {
        return 0;
    }
    return a->m_54 > b->m_54;
}

// ---------------------------------------------------------------------------
// 0x1847a0 - trivial setter `m_70 = arg;`. __thiscall, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x001847a0, 0xa)
void B_1847a0::Set(i32 v) {
    m_70 = v;
}

// ---------------------------------------------------------------------------
// 0x17fc40 - `if(m_50) RezFree(m_50);` (no zero-out). __thiscall, 0 args.
// ---------------------------------------------------------------------------
RVA(0x0017fc40, 0x11)
void B_17fc40::Free() {
    if (m_50) {
        RezFree(m_50);
    }
}

// ---------------------------------------------------------------------------
// 0x184fb0 - __cdecl forward `G(0, a, b);` to 0x184fd0.
// ---------------------------------------------------------------------------
void Sub_184fd0(i32, i32, i32); // 0x184fd0, no body
RVA(0x00184fb0, 0x15)
void Fwd_184fb0(i32 a, i32 b) {
    Sub_184fd0(0, a, b);
}

// ---------------------------------------------------------------------------
// 0x134360 / 0x1346d0 - DirectInput device-config teardown: free the +0x2a0
// buffer, then chain the base ReleaseDevices (0x1342b0). Two identical leaves.
// ---------------------------------------------------------------------------
RVA(0x00134360, 0x33)
void DevCfg::Free360() {
    if (m_2a0) {
        RezFree(m_2a0);
        m_2a0 = 0;
        m_2a4 = 0;
    }
    ReleaseBase();
}
RVA(0x001346d0, 0x33)
void DevCfg::Free6d0() {
    if (m_2a0) {
        RezFree(m_2a0);
        m_2a0 = 0;
        m_2a4 = 0;
    }
    ReleaseBase();
}

// ---------------------------------------------------------------------------
// 0x145e00 - parity test: returns (popcount(x) == 1). __cdecl, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x00145e00, 0x26)
i32 PopcountIsOne_145e00(i32 x) {
    i32 c = 0;
    i32 i = 0x20;
    do {
        if ((x & 1) == 1) {
            c++;
        }
        x >>= 1;
    } while (--i);
    return c == 1;
}

// ---------------------------------------------------------------------------
// 0x1413b0 - manual-vtable dispatch `(*m_8->vtbl[0x80])(m_8, 0)`. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x001413b0, 0xf)
void Owner1413::Thunk() {
    m_8->Op(0);
}

// ---------------------------------------------------------------------------
// CDdObArray mode-table search (m_4b8 = Entry*[], m_4bc = count).
// 0x1434c0 FindIndex (exact 3-key match), 0x143470 FindLast (>= range match).
// ---------------------------------------------------------------------------
RVA(0x001434c0, 0x45)
i32 ModeArr::FindIndex(i32 k0, i32 k1, i32 k2) {
    for (i32 i = 0; i < m_4bc; i++) {
        DdEntry* e = m_4b8[i];
        if (e->m_c == (u32)k0 && e->m_8 == (u32)k1 && e->m_54 == k2) {
            return i;
        }
    }
    return -1;
}
RVA(0x00143470, 0x47)
i32 ModeArr::FindLast(u32 k0, u32 k1, i32 k2) {
    i32 r = -1;
    for (i32 i = m_4bc - 1; i >= 0; i--) {
        DdEntry* e = m_4b8[i];
        if (e->m_c >= k0 && e->m_8 >= k1 && e->m_54 == k2) {
            r = i;
        }
    }
    return r;
}

// ---------------------------------------------------------------------------
// 0x13dee0 - `m_1c = v; if(v > 0) m_28 = 1000 / v;` (CFileImage frame timing).
// __thiscall, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x0013dee0, 0x1b)
void B_13dee0::Set(i32 v) {
    m_1c = v;
    if (v > 0) {
        m_28 = 1000 / v;
    }
}

// ---------------------------------------------------------------------------
// 0x13ee30 - COM wait-flip loop: `while(m_8->Flip(2) == DDERR_WASSTILLDRAWING);`.
// IDirectDrawSurface-style manual vtable, slot 0x48. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x0013ee30, 0x29)
void B_13ee30::WaitFlip() {
    while (m_8->Flip(2) == 0x8876021c) {
    }
}

// ---------------------------------------------------------------------------
// 0x151e70 - clear: zero m_10, release the +0x14 buffer (+ m_178), scalar-delete
// the +0x18 object (vtbl slot 0, arg 1), zero m_170. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00151e70, 0x3b)
void B_151e70::Clear() {
    m_10 = 0;
    if (m_14) {
        RezFree(m_14);
        m_14 = 0;
        m_178 = 0;
    }
    if (m_18) {
        m_18->Destroy(1);
        m_18 = 0;
    }
    m_170 = 0;
}

// ---------------------------------------------------------------------------
// 0x166810 - destroy a singly-linked list of nodes (link at +0, payload at +8,
// scalar-deleted via vtbl slot 1 arg 1), then RemoveAll the +0x1dc array (0x1b5a0b).
// __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00166810, 0x32)
void B_166810::Clear() {
    Node166810* n = m_head;
    while (n) {
        Node166810* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            cur->m_payload->Destroy(1);
        }
    }
    ((CObArray*)&m_1dc)->RemoveAll();
}

// ---------------------------------------------------------------------------
// External rez helpers (reloc-masked rel32).
// ---------------------------------------------------------------------------
extern "C" i32 RezItmProbe(void* h);  // 0x125b50
extern "C" i32 RezDirLookup(void* h); // 0x18ccd0

// ---------------------------------------------------------------------------
// 0x13c8a0 - CRezItm scan retry loop: probe m_handle; if it yields, ask the
// +0xc owner's vtbl[2] whether to keep retrying. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x0013c8a0, 0x45)
i32 RezItm::Scan() {
    m_20 = -1;
    if (m_handle) {
        i32 found;
        do {
            if (RezItmProbe(m_handle) == 0) {
                found = 1;
            } else {
                found = 0;
                if (m_owner->v2() == 0) {
                    return 0;
                }
            }
        } while (!found);
        return found;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x13c8f0 - CRezDir check: lookup m_10; if found return 1; else dispatch the
// virtual slot 4 (m_14, m_18, 0) and normalize to bool. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x0013c8f0, 0x41)
i32 RezDir::Check() {
    m_20 = -1;
    if (!m_10) {
        return 0;
    }
    if (RezDirLookup(m_10) != -1) {
        return 1;
    }
    return v4(m_14, m_18, 0) != 0;
}

// ---------------------------------------------------------------------------
// CDdObArray neighbour lookup: 0x143510 forward, 0x143590 backward. Both run
// FindIndex then scan toward the array end / start for a same-m_54 entry, writing
// {m_c, m_8} (or {-1,-1}) to the out pair. __thiscall, 4 args.
// ---------------------------------------------------------------------------
// @early-stop
// ~82.5% regalloc wall: body + guards + FindIndex call byte-exact; in the scan
// loop retail pins the strength-reduced iterator pointer in edx and the loaded
// entry in ecx, while MSVC5 here swaps them (entry in edx), cascading into the
// found-path field reads. No source spelling (index, hoisted-base, explicit
// pointer-walk) flips the pair; logic complete.
RVA(0x00143510, 0x71)
void ModeArr::FindFwd(Pair2* out, i32 k0, i32 k1, i32 k2) {
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_4bc) {
        idx++;
        if (idx < m_4bc) {
            for (; idx < m_4bc; idx++) {
                DdEntry* e = m_4b8[idx];
                if (e->m_54 == k2) {
                    out->a = e->m_c;
                    out->b = e->m_8;
                    return;
                }
            }
        }
    }
    out->a = -1;
    out->b = -1;
}
// @early-stop
// ~72.8% regalloc wall: same iterator/entry register swap as FindFwd (mirror,
// descending scan). Logic complete.
RVA(0x00143590, 0x7e)
void ModeArr::FindBack(Pair2* out, i32 k0, i32 k1, i32 k2) {
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_4bc) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                DdEntry* e = m_4b8[idx];
                if (e->m_54 == k2) {
                    out->a = e->m_c;
                    out->b = e->m_8;
                    return;
                }
            }
        }
    }
    out->a = -1;
    out->b = -1;
}

// ---------------------------------------------------------------------------
// 0x138f20 - DSound voice gate: if vtbl[8]() and !Helper(0x138f60), clear m_44
// and dispatch vtbl[9](m_4c, m_48); return success. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00138f20, 0x3a)
i32 Snd138f20::Gate() {
    if (!v8()) {
        return 0;
    }
    if (Helper()) {
        return 0;
    }
    m_44 = 0;
    v9(m_4c, m_48);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x16be60 - ButeMgr helper append: if Ready(), Emit(&g_emptyString, arg) then
// Flush(); return this. __thiscall, 1 arg.
// ---------------------------------------------------------------------------
extern "C" char g_emptyString[]; // _g_emptyString @0x6293f4
RVA(0x0016be60, 0x2a)
C16be60* C16be60::Append(i32 arg) {
    if (Ready()) {
        Emit(g_emptyString, arg);
        Flush();
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x151d20 - notify a hooked callback: stash/replace m_7c->m_1c with arg, invoke
// the +0x10 callback(this), restore m_1c if unchanged. __thiscall, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x00151d20, 0x3a)
i32 B_151d20::Notify(void* arg) {
    Cb151d20* p = m_7c;
    if (!p) {
        return 0;
    }
    void* saved = p->m_1c;
    p->m_1c = arg;
    m_7c->fn(this);
    if (m_7c->m_1c == arg) {
        m_7c->m_1c = saved;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Global-object tail-forwards: load the global's address into ecx and tail-jump
// the shared array teardown (0x1b4f0b). The DATA globals are named; the shared
// callee is reloc-masked.
// ---------------------------------------------------------------------------
extern CImageCache g_imageCache; // 0x653c88
RVA(0x0013e070, 0xa)
void ClearImageCache_13e070() {
    g_imageCache.RemoveAll();
}

extern CDdObArray g_modeArray; // 0x683ec8
RVA(0x00141c80, 0xa)
void ClearModeArray_141c80() {
    g_modeArray.RemoveAll();
}

// ---------------------------------------------------------------------------
// CImageOwned apply/setup cluster (vptr slot 8 = +0x20 transform, slot 10 = +0x28
// commit). 0x13e0a0 Apply copies the 0x6c-byte source block into m_10 then runs
// the transform; 0x148cc0 / 0x148b50 forward into it.
// ---------------------------------------------------------------------------
RVA(0x0013e0a0, 0x27)
i32 ImgOwned::Apply(i32 mode, const void* src) {
    if (src) {
        m_transform = *(const Blk6c*)src;
    }
    return v8(mode);
}
RVA(0x00148cc0, 0x18)
i32 ImgOwned::Forward(i32 a0, const void* a1) {
    return Apply(a0, a1) != 0;
}
RVA(0x00148b50, 0x2c)
i32 ImgOwned::Commit(i32 a0, const void* a1) {
    if (Apply(a0, a1) == 0) {
        return 0;
    }
    v10();
    return 1;
}

// ---------------------------------------------------------------------------
// 0x13dec0 - millisecond frame-pacing busy-wait via the timeGetTime function
// pointer global: spin until now passes start+ms (unsigned, overflow-guarded).
// Reached __thiscall from RezMgr::UpdateClock (`mov ecx,esi(this); call`), NOT the
// __stdcall free function Ghidra mislabeled (Delay_13dec0@@YG). Relabeled to the
// __thiscall member (RezMgr::SpinWaitUntil) so UpdateClock's SpinWaitUntil call
// reloc pairs with this definition (folds the placeholder symbol). `this` is unused
// by the body (ecx ignored), so the emitted bytes are identical to the stdcall form;
// the fn-ptr is cached in a callee-saved reg across the loop. Only a minimal RezMgr
// view is declared here - the full RezMgr.h is /O2-sensitive when pulled into other
// TUs, so it is not included.
// ---------------------------------------------------------------------------
extern "C" u32(WINAPI* g_pTimeGetTime)(); // _g_pTimeGetTime @0x6c4650
// @early-stop
// ~83.9% regalloc wall: body byte-exact, but retail pins the cached fn-ptr in edi
// and the deadline in esi (pushing both callee-saves upfront), while MSVC5 swaps
// them (fn-ptr in esi, deadline in edi, edi shrink-wrapped). No source spelling
// flips the esi/edi pair; logic complete.
RVA(0x0013dec0, 0x20)
void RezMgr::SpinWaitUntil(i32 ms) {
    u32(WINAPI * fn)() = g_pTimeGetTime;
    u32 now = fn();
    u32 end = now + (u32)ms;
    if (now <= end) {
        do {
            now = fn();
        } while (now <= end);
    }
}
