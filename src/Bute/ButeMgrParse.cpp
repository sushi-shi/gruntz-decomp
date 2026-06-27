// ButeMgrParse.cpp - CButeMgr::Parse(CString, int) @0x3cc20.
//
// The .att file-parse entry point: opens a CRT istream-derived source stream
// over the named file, checks it opened OK (ios::state & (failbit|badbit)),
// resets the three keyed stores, runs the recursive group parse, then closes +
// deletes the stream. /GX EH-framed: the by-value CString `filename` param is
// callee-destroyed, and the raw `new` storage needs delete-on-throw during the
// stream ctor (state 0 -> 1 -> 0 -> -1).
//
// The stream is a multiply/virtually-derived CRT iostream class: its `ios`
// virtual base carries the state word at +8 (the `& 6` open-failure probe) and
// the vtable whose slot-0 scalar-deleting dtor runs the `delete`. Modeled as a
// real virtual-base hierarchy so MSVC lowers the vbtable lookups + the vbase-
// adjusted virtual delete; the engine ctor/Sync (0x169fb0 / 0x16a3b0) are
// external no-body callees (reloc-masked).
#include <Bute/ButeMgr.h>
#include <rva.h>

// Global operator new (engine NAFXCW); external/no-body so the `push 0x5c;
// call ??2; add esp,4` shape falls out reloc-masked.
void* operator new(u32 n);

// The shared default-attribute descriptor pointer the stream ctor takes (the
// value stored at 0x5f03e0 is pushed as the 3rd ctor arg).
DATA(0x001f03e0)
extern "C" void* g_pButeDefaults; // 0x5f03e0

// ---------------------------------------------------------------------------
// The CRT `ios` virtual base of the source stream. Only the load-bearing
// shape: the vptr (slot-0 scalar-deleting dtor drives `delete`), the +8 state
// word (`& (failbit|badbit)` open-fail probe), and the 0x50-byte size (so the
// derived class's vbtable[1] resolves to +0xc and sizeof(stream) == 0x5c).
// All members external/opaque.
struct ButeIos {
    virtual ~ButeIos(); // vptr @ +0
    void* m_bp;         // +0x04  streambuf*
    int m_state;        // +0x08  io_state (& 6 == failbit|badbit)
    char m_pad0c[0x50 - 0xc];
};

// The file source stream `new`-d at +0xa0: vbptr @ +0, two derived dwords, then
// the `ios` virtual base @ +0xc -> sizeof 0x5c. The ctor (0x169fb0) takes the
// filename, an open-mode word (0x21), and the defaults pointer; MSVC appends the
// hidden most-derived vbase flag (=1). Sync (0x16a3b0) finalizes before delete.
struct ButeFileStream : virtual ButeIos {
    int m_d4;                                                       // +0x04
    int m_d8;                                                       // +0x08
    ButeFileStream(const char* fileName, int mode, void* defaults); // 0x169fb0
    void Sync();                                                    // 0x16a3b0
    ~ButeFileStream(); // external; the delete runs the vbase vtable's slot-0
};

// ---------------------------------------------------------------------------
// CButeMgr::Parse(CString, int)
// @early-stop
// 98.84% - logic + instruction-selection byte-exact (verified base-vs-target
// llvm-objdump); two residuals, both proven artifacts:
//   (1) the /GX prologue `push <scopetable>`: base references its local unwind
//       table `$L17759 + 0`, retail references the shared `Unwind@005d9ca8 +
//       0xb`. objdiff masks the symbol but the addend (0x0 vs 0xb) differs -
//       the per-unit unwind-record-layout wall (retail packed this fn's unwind
//       record at +0xb of a shared table; an isolated single-fn unit emits it
//       at +0 of a local symbol; not source-steerable).
//   (2) one `mov ecx,esi` (ParseGroup's `this`-setup) scheduled at slot 1 of
//       the third inlined Reset()'s store run (mine) vs slot 3 (retail) - a
//       pure MSVC5 list-scheduler tie-break; same opcode/operands, identical
//       everywhere else. Tried moving the `result=true` init + an explicit
//       tree pointer; neither flips it (one regressed to 96.3%).
RVA(0x0003cc20, 0x14e)
bool CButeMgr::Parse(CString filename, int streamBase) {
    ButeFileStream* s = new ButeFileStream(filename, 0x21, g_pButeDefaults);
    m_stream = s;
    if ((s->m_state & 6) != 0) {
        return false;
    }

    Init();
    m_streamBase = streamBase;
    m_str108 = filename;

    m_tree.Reset();
    m_tree48.Reset();
    m_tree74.Reset();

    bool result = true;
    if (!ParseGroup()) {
        m_0d = 1;
        result = false;
    }

    ((ButeFileStream*)m_stream)->Sync();
    delete (ButeFileStream*)m_stream;
    return result;
}
