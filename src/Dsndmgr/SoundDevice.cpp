// SoundDevice.cpp - the DirectSound *device* manager's lifecycle/teardown run
// (C:\Proj\Dsndmgr\DSNDMGR.CPP, retail vftable 0x5ef6c4). Owns a list of
// per-buffer DirectSoundMgr wrappers (head @ +0x04, chained through each
// wrapper's +0x04 link with the MFC POSITION +4 bias), the IDirectSound device
// (+0x14) and primary buffer (+0x84). The teardown releases every buffer's
// voices + COM interface, then the primary + device.
//
// Trace called this class "MinervaInner"; its 0x5ef6c4 vtable proves it is a
// distinct class from the buffer wrapper DirectSoundMgr (0x5ef6b8). Field names
// are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/SoundDevice.h>
#include <Rez/RezMgr.h> // RezFree - the engine resource deallocator (0x1b9b82)
#include <rva.h>

// The __FILE__ string the device wrappers pass to GetErrorString (the shared
// DSNDMGR.CPP $SG pooled constant, 0x619ef8).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// The abstract-base ("pure") vftable (0x5ef6c8) a freed sample's vptr is restamped
// to before RezFree - a transitional reloc-masked DIR32 store.
DATA(0x001ef6c8)
extern void* const g_PureVtbl[];

// The device's voice/channel sub-list reap helper (0x136f60, __thiscall on the
// +0x0c list head): unlink + free every voice matching (arg, mask). Same engine
// helper DirectSoundMgr.cpp models as DSoundVoiceList::Reap.
struct SoundVoiceList {
    void* m_head;
    void* m_tail;
    void Reap(void* node, i32 mask); // 0x136f60
};

// The intrusive buffer-list unlink helper (0x1391e0, __thiscall on the +0x04
// list head): unlink one node given its anchor. Same as DSoundCloneList::Unlink.
struct SoundBufList {
    void* m_head;
    void* m_tail;
    void Unlink(void* anchor); // 0x1391e0
};

// The device class's retail vftable (0x5ef6c4), stamped by ~SoundDevice at entry
// - a transitional reloc-masked DIR32 store while the class's virtuals aren't all
// matched (so the class stays non-polymorphic and the compiler emits no vtable).
DATA(0x001ef6c4)
extern void* const g_SoundDeviceVtbl[];

// SoundBuf::StopAndRewind (0x135380) / StopAllClones (0x136150) are external
// DirectSoundMgr buffer methods (defined in DirectSoundMgr.cpp); declared with no
// body here so their __thiscall calls reloc-mask as rel32.

// ---------------------------------------------------------------------------
// SoundDevice::~SoundDevice (0x136500, __thiscall, /GX EH frame). Stamp the
// device vptr, then if initialized run the full teardown.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the vptr-stamp + `if(m_78) Shutdown()` body is byte-exact, but the retail /GX EH
// frame (push -1 / fs:0 setup / trylevel) comes from a non-trivial base subobject
// the manual-vptr non-polymorphic model can't emit. Same wall as DirectSoundMgr::
// ~DirectSoundMgr (0x135bb0). Defer to the final sweep when the full base+vtable
// is modeled; converting now would regress the 3 exact siblings.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    *(void**)this = (void*)g_SoundDeviceVtbl;
    if (m_78) {
        Shutdown();
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::Shutdown (0x136690, __thiscall). Release every owned buffer
// wrapper (RemoveBuffer drains its voices, releases its COM buffer + unlinks +
// destroys it), then the primary buffer and the device, then clear the flag.
RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_78) {
        SoundBuf* node = m_04_head ? (SoundBuf*)((char*)m_04_head - 4) : 0;
        while (node) {
            RemoveBuffer(node);
            node = m_04_head ? (SoundBuf*)((char*)m_04_head - 4) : 0;
        }
        if (m_84) {
            m_84->vtbl->Release(m_84);
        }
        m_14->vtbl->Release(m_14);
    }
    m_78 = 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::RemoveBuffer (0x136d80, __thiscall, 1 arg). Reap the buffer's
// queued voices, release its IDirectSoundBuffer, unlink it from the owned-buffer
// list, then run its scalar-deleting destructor.
RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(SoundBuf* node) {
    if (m_78) {
        ((SoundVoiceList*)&m_0c)->Reap(node, 0xffff);
        if (node->m_buf0c) {
            node->m_buf0c->vtbl->Release(node->m_buf0c);
            node->m_buf0c = 0;
        }
        ((SoundBufList*)&m_04_head)->Unlink(node ? &node->m_link : 0);
        if (node) {
            ((DSoundCloneBase*)node)->ScalarDtor(1);
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::StopAll (0x136de0, __thiscall). Walk the owned-buffer list,
// StopAndRewind + StopAllClones on each.
RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_78) {
        SoundBuf* node = m_04_head ? (SoundBuf*)((char*)m_04_head - 4) : 0;
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = node->m_link ? (SoundBuf*)((char*)node->m_link - 4) : 0;
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::FreeSamples (0x136ed0, __thiscall). Walk the cached-sample list
// (+0x0c, biased +4 links): for each, run its slot-1 "free" virtual, unlink it,
// restamp its vptr to the pure base, and RezFree it. Returns 1.
// @early-stop
// regalloc/early-out scheduling wall: retail reserves all 4 callee-saved regs
// (ebx/ebp/esi/edi) in the prologue and runs the `if(!m_78) return 0` early-out
// IN-frame (popping all 4), while MSVC here emits a leaner frameless early-out
// before the saves. The loop body (slot-1 Free, neg/sbb/and arg, vptr restamp,
// RezFree) is byte-exact; only the prologue/early-out register scheduling shifts.
// 77% on a documented regalloc wall - logic complete, deferred to the final sweep.
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_78 == 0) {
        return 0;
    }
    SoundSample* node = m_0c ? (SoundSample*)((char*)m_0c - 4) : 0;
    while (node) {
        SoundSample* next = node->m_link ? (SoundSample*)((char*)node->m_link - 4) : 0;
        node->Free();
        ((SoundBufList*)&m_0c)->Unlink(node ? &node->m_link : 0);
        if (node) {
            *(void**)node = (void*)g_PureVtbl;
            RezFree(node);
        }
        node = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SoundDevice::SetPrimaryFormat (0x1371a0, __thiscall, 1 arg). Ensure the primary
// buffer exists, then set its WAVEFORMATEX; report a failing HRESULT and bail.
RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_78 == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_84->vtbl->SetFormat(m_84, fmt) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}
