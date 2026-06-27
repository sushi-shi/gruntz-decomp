// CSeverusWorker.h - the DDrawMgr "severus worker" (placeholder name; Ghidra
// g_severusWorker* / CSeverusWorker_016500). A large (>0x8694 B) movie/stream
// decode object: its main cluster (0x17b500..0x17c790) wraps DirectDrawCreate,
// ShowCursor and the Smacker decoder around an embedded decode store at +0x540
// (CFile @ +0x124, CByteArray @ +0x138) and a Rez-owned scratch embed at +0x868c.
//
// Modeled with the manual-vtable-stamp device (no real `virtual`) so the matched
// methods don't emit a divergent compiler vtable. Offsets + emitted bytes are the
// load-bearing fact; field names are placeholders.
//
// Two reconstructed entry points so far (separate retail TUs):
//   0x17c6f0  CSeverusWorker::Open   (src/Gruntz/CSeverusWorker.cpp, frameless)
//   0x038fc0  CSeverusWorker::~CSeverusWorker (src/Gruntz/CSeverusWorkerEh.cpp, /GX)
#ifndef GRUNTZ_CSEVERUSWORKER_H
#define GRUNTZ_CSEVERUSWORKER_H

#include <Ints.h>

// The Rez heap free (reloc-masked rel32 callee, __cdecl 1 arg). 0x1b9b82.
extern "C" void RezFree(void* p);

// An MFC CFile-shaped member: its non-trivial dtor is the reloc-masked engine
// ~CFile (0x1bf121) so the /GX member-teardown trylevel falls out
// (eh-dtor-model-members-as-destructible).
struct SevFile {
    void Dtor_1bf121(); // ~CFile (reloc-masked rel32 callee)
    ~SevFile() { Dtor_1bf121(); }
};

// An MFC CByteArray-shaped member: dtor -> the reloc-masked engine ~CByteArray
// (0x1b4b76).
struct SevByteArray {
    void Dtor_1b4b76(); // ~CByteArray (reloc-masked rel32 callee)
    ~SevByteArray() { Dtor_1b4b76(); }
};

// The decode store embedded at worker+0x540. Abort() (0x17b570) tears down the
// active decode; the CFile/CByteArray members destruct after it.
struct CSeverusStore {
    void* m_vptr;        // +0x00  store vptr (Abort gates on it)
    char m_pad4[0x124 - 0x04];
    SevFile m_124;       // +0x124  decode CFile
    char m_pad125[0x138 - 0x125];
    SevByteArray m_138;  // +0x138  decode CByteArray
    char m_pad139[0x200 - 0x139];

    i32 Begin();      // 0x17b510  prepare/probe (returns nonzero on ready)
    i32 OpenA(i32 a); // 0x17b5f0  open low-res source
    void Abort();     // 0x17b570  tear down the active decode
    i32 OpenB(i32 a); // 0x17b840  open high-res source
    ~CSeverusStore();
};

// The Rez-owned scratch embed at worker+0x868c: a CSeverusBase-shaped subobject
// (vptr + one owned buffer). Its teardown stamps its own vtable, RezFree's the
// buffer, then restores the shared severus base dtor vtable.
struct CSeverusEmbed {
    void* m_vptr; // +0x00 (worker+0x868c)
    void* m_4;    // +0x04 (worker+0x8690)  Rez-owned buffer
    ~CSeverusEmbed();
};

class CSeverusWorker {
public:
    i32 Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x17c6f0
    i32 OpenHi(i32 src, i32 a2, i32 a3, i32 a4, i32 a5);      // 0x17c630 (sibling)
    void Teardown();                                          // 0x17c510 (sibling)
    ~CSeverusWorker();                                        // 0x038fc0

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    void* m_vptr; // +0x00
    i32 m_4;      // +0x04  active flag (Open bails when 0)
    char m_pad8[0x540 - 0x08];
    CSeverusStore m_540; // +0x540
    char m_pad740[0x868c - (0x540 + 0x200)];
    CSeverusEmbed m_868c; // +0x868c
};

#endif // GRUNTZ_CSEVERUSWORKER_H
