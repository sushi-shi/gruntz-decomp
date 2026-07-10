// GruntzCommand.cpp - the command-pattern class family (CGruntzCommand base +
// CGruntzSingleCommand / CGruntzMultiCommand leaves). See GruntzCommand.h for
// the hierarchy + layout. Names are placeholders; offsets + code bytes are
// load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CGruntzCommand::~CGruntzCommand() (??_G) 0x024330 - slot-0 scalar-deleting
//       dtor: restore vftable, (flag&1) -> operator delete(this).  BYTE-EXACT.
//   CGruntzSingleCommand::Allocate()  0x024220 - new-or-recycle allocator:
//       recycle-list tail-call else operator new(0x14) + inline-ctor vftable
//       store.  (Retail labels this RVA ??0CGruntzSingleCommand; the body is the
//       allocator, not a plain ctor - see the allocator note below.)
//   CGruntzMultiCommand::Allocate()   0x024360 - same shape, Multi list/vftable.
//
// NOT matched (see report): the two 7-byte void-thiscall vtable-set helpers at
// 0x0242f0 / 0x024430 (?CGruntzCommand_0242f0 / _024430). Their retail bytes are
// a bare `mov [this],&vftable; ret` (void, no eax-return, no null guard) - an
// out-of-line materialization of the inlined base-vftable store that MSVC 5.0
// will NOT emit as a standalone COMDAT from the canonical class under the locked
// flags (a real ??0 emits `mov eax,ecx`; placement-new emits a null guard).
// They stay in the src/Stub/ backlog as the documented COMDAT-duplication case.
#include <Win32.h> // windows.h (USER32) for the numeric settings DialogProc (0x92ab0)
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/SerialArchive.h> // the shared archive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/WwdGameReg.h>    // the canonical WwdGameReg singleton (g_gameReg)
#include <rva.h>

// CGruntzCmdTarget::Exec IS CCmdHandler::Dispatch @0xd1b60 (7 args); local decl.
class CCmdHandler {
public:
    i32 Dispatch(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g);
};

// The game registry singleton (canonical <Gruntz/WwdGameReg.h>). Save/Load no-op
// unless its +0x30 active-game gate (m_world) is non-null (same gate the cmd-mgr
// taps); this TU only null-tests it, so the m_world facet type is irrelevant here.
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// CGruntzCommand::~CGruntzCommand() - the slot-0 scalar-deleting dtor (??_G).
// Restore the vftable, then (if the low bit of the hidden flags arg is set)
// operator delete(this). Empty body (no members, no base) - MSVC synthesizes
// the deleting thunk from `virtual ~CGruntzCommand() {}` in the header. The
// thunk has no source body so it cannot carry an RVA() attribute; pin the
// deleting-dtor symbol by mangled name here.
// @rva-symbol: ??_GCGruntzCommand@@UAEPAXI@Z 0x00024330 0x20

// Out-of-line vtable anchors (the slots NOT reconstructed here) so the
// CGruntzCommand vftable is emitted in this TU (the ctor/dtor reference it).
// Bodies are placeholders.
i32 CGruntzCommand::Serialize(CSerialArchive*, i32, i32, i32) {
    return 1;
}
i32 CGruntzCommand::Save(CSerialArchive*) {
    return 0;
}
i32 CGruntzCommand::Load(CSerialArchive*) {
    return 0;
}
i32 CGruntzCommand::Vslot05() {
    return 1;
}
char CGruntzCommand::GetTag() {
    return 0;
}
i32 CGruntzCommand::Parse(void*, i32) {
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParams() - 0x023e20 (vtable slot 4). The base "set the
// five scalar params" implementation: store kind/sub bytes + two words, return
// 1. Inherited unchanged by both leaves (slot 4 = this in all three vtables).
// ---------------------------------------------------------------------------
RVA(0x00023e20, 0x2f)
i32 CGruntzCommand::SetParams(char a0, char a1, char a2, i16 a3, i16 a4) {
    m_4 = a0;
    m_5 = a1;
    m_6 = a2;
    m_8 = a3;
    m_a = a4;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParamsEx() - 0x023e60. Delegate the five scalar params to
// the base SetParams (a direct, devirtualized call), then store the +0x10/+0x11
// byte pair; return 1 (or 0 if SetParams failed).
// ---------------------------------------------------------------------------
RVA(0x00023e60, 0x42)
i32 CGruntzCommand::SetParamsEx(char a0, char a1, char a2, i16 a3, i16 a4, char a5, char a6) {
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    m_10 = a5;
    m_11 = a6;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetMaskFromList() - 0x023ed0. Validate (buf!=0, count<=0x10),
// delegate the five scalar params to SetParams, then OR a 16-bit flag mask at
// +0x10 from `count` indices in `buf` (m_10word |= 1<<buf[i]). Returns 1/0.
// ---------------------------------------------------------------------------
RVA(0x00023ed0, 0x83)
i32 CGruntzCommand::SetMaskFromList(char a0, char a1, char a2, i16 a3, i16 a4, i32 count, u8* buf) {
    if (!buf) {
        return 0;
    }
    if ((u8)count > 0x10) {
        return 0;
    }
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    *(u16*)&m_10 = 0;
    for (i32 i = 0; i < (count & 0xff); i++) {
        *(u16*)&m_10 |= g_cmdBitTable[buf[i]];
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Parse() - 0x023f90 (vtable slot 7). The inverse of Pack:
// deserialize the flat byte buffer back into the five scalar params + m_10, plus the
// conditional m_11 byte (when m_5 >= 8). The tag byte is skipped. Returns the byte
// count consumed. The `len` arg is unused (the record is self-delimiting).
// @early-stop
// walker-register coin-flip (~86.4%, docs/patterns/zero-register-pinning.md family):
// the running-pointer deserialize is byte-faithful instruction-for-instruction (the
// tag skip, the m_4/m_5/m_6 byte reads, the m_8/m_a word reads, m_10, the m_5>=8
// m_11 tail, and the buf-start byte count), verified vs target with sema disasm
// --diff. The sole residual is which register holds the running buffer pointer:
// retail loads `data` into eax BEFORE `push esi` and walks eax (esi=start copy); cl
// pins the walker in esi and derives eax, cascading the modrm register field through
// every read. Not source-steerable (three spellings + the permuter: no change).
// ---------------------------------------------------------------------------
RVA(0x00023f90, 0x48)
i32 CGruntzSingleCommand::Parse(void* data, i32 /*len*/) {
    char* buf = (char*)data + 1; // skip the tag byte
    m_4 = *buf++;
    m_5 = *buf++;
    m_6 = *buf++;
    m_8 = *(i16*)buf;
    buf += 2;
    m_a = *(i16*)buf;
    buf += 2;
    m_10 = *buf++;
    // read m_5 as an unsigned byte (retail `cmp byte,8; jb`) via the &-address form so
    // the codegen is identical to an unsigned member conversion without a member cast.
    if (*(u8*)&m_5 >= 8) {
        m_11 = *buf++;
    }
    return buf - (char*)data;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Parse() - 0x024000 (vtable slot 7). The multi-target twin: the
// +0x10 field is read as a full 16-bit flag mask (a WORD) rather than the conditional
// byte pair. Returns the byte count consumed.
// @early-stop
// walker-register coin-flip (~83.2%): same wall as CGruntzSingleCommand::Parse above -
// the running-pointer deserialize is byte-faithful except the walker register (retail
// eax, cl esi). Not source-steerable.
// ---------------------------------------------------------------------------
RVA(0x00024000, 0x3e)
i32 CGruntzMultiCommand::Parse(void* data, i32 /*len*/) {
    char* buf = (char*)data + 1; // skip the tag byte
    m_4 = *buf++;
    m_5 = *buf++;
    m_6 = *buf++;
    m_8 = *(i16*)buf;
    buf += 2;
    m_a = *(i16*)buf;
    buf += 2;
    *(i16*)&m_10 = *(i16*)buf;
    buf += 2;
    return buf - (char*)data;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Pack() - 0x024050. Serialize the command into a flat
// byte buffer: the tag byte (vtable slot 6), the kind/sub/aux byte triple
// (m_4/m_5/m_6), the two words (m_8/m_a), the m_10 byte, and - only when the kind
// byte m_5 is >= 8 - the m_11 byte. Returns the byte count written. The buffer
// pointer walks with single-byte post-increments then word stores (the natural
// running-pointer codegen).
// ---------------------------------------------------------------------------
RVA(0x00024050, 0x57)
i32 CGruntzSingleCommand::Pack(char* buf, i32 /*unused*/) {
    char* start = buf;
    *buf = (char)GetTag();
    *++buf = m_4;
    *++buf = m_5;
    *++buf = m_6;
    char* w = buf + 1;
    *(i16*)w = m_8;
    w += 2;
    *(i16*)w = m_a;
    w += 2;
    *w = m_10;
    w++;
    if ((u8)m_5 >= 8) {
        *w = m_11;
        w++;
    }
    return w - start;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Pack() - 0x0240d0. The multi-target twin: same tag + the
// five scalar params, but the +0x10 field is written as a full 16-bit flag mask
// (a WORD) rather than the conditional byte pair. Returns the byte count written.
// ---------------------------------------------------------------------------
RVA(0x000240d0, 0x4d)
i32 CGruntzMultiCommand::Pack(char* buf, i32 /*unused*/) {
    char* start = buf;
    *buf = (char)GetTag();
    *++buf = m_4;
    *++buf = m_5;
    *++buf = m_6;
    char* w = buf + 1;
    *(i16*)w = m_8;
    w += 2;
    *(i16*)w = m_a;
    w += 2;
    *(i16*)w = *(i16*)&m_10;
    w += 2;
    return w - start;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyOne() - 0x024140. If p!=0, unpack the params and call the
// CPlay executor once with index slot = m_10. Returns its result (0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024140, 0x35)
i32 CGruntzCommand::ApplyOne(CGruntzCmdTarget* p) {
    if (!p) {
        return 0;
    }
    return ((CCmdHandler*)p)->Dispatch(m_4, m_10, m_5, m_8, m_a, m_11, m_6);
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyMask() - 0x024190. For each of the 16 bit positions set
// in the +0x10 flag mask, call the executor with that index; AND the results
// (return 1 only if every call succeeded; 0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024190, 0x6c)
i32 CGruntzCommand::ApplyMask(CGruntzCmdTarget* p) {
    if (!p) {
        return 0;
    }
    i32 ok = 1;
    for (i32 i = 0; i < 16; i++) {
        if (g_cmdBitTable[i] & *(u16*)&m_10) {
            if (!((CCmdHandler*)p)->Dispatch(m_4, (char)i, m_5, m_8, m_a, 0, m_6)) {
                ok = 0;
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Allocate() - 0x024220.
//
// new-or-recycle allocator: if the per-class recycle list is non-empty
// (g_singleCmdCount != 0), tail-call g_singleCmdList.RemoveTail() to reuse a
// node; otherwise operator new(0x14) a fresh object and (if non-null) run the
// inline ctor, which only stamps the leaf vftable. Returns the object (or 0).
// ---------------------------------------------------------------------------
RVA(0x00024220, 0x2b)
CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    if (g_singleCmdCount) {
        return (CGruntzSingleCommand*)g_singleCmdList.RemoveTail();
    }
    return new CGruntzSingleCommand;
}

// The CGruntzCommand base vftable (0x5e9674; == ??_7CGruntzCommand@@6B@ this TU
// emits). The two out-of-line stamps store it into [this]; the operand reloc-
// masks against the base vftable via this DATA-named extern (re-homed from
// src/Stub/CGruntzCommand.cpp).

// CGruntzCommand::CGruntzCommand_0242f0 (0x000242f0) is now an inline member in the header.

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Allocate() - 0x024360. Same shape, Multi list/vftable.
// ---------------------------------------------------------------------------
RVA(0x00024360, 0x2b)
CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    if (g_multiCmdCount) {
        return (CGruntzMultiCommand*)g_multiCmdList.RemoveTail();
    }
    return new CGruntzMultiCommand;
}

// CGruntzCommand::CGruntzCommand_024430 (0x00024430) is now an inline member in the header.

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::FreeAll() - 0x024450. Drain the per-class recycle list
// (g_singleCmdList): while non-empty, RemoveTail() a node and `delete` it (the
// virtual scalar-deleting dtor with flag 1). A static method (no this). (Retail
// FLIRT-misattributed this RVA as NetUnattributed_024450; it is the single-cmd
// twin of CGruntzMultiCommand::FreeAll below.)
// ---------------------------------------------------------------------------
RVA(0x00024450, 0x29)
void CGruntzSingleCommand::FreeAll() {
    while (g_singleCmdCount) {
        CGruntzCommand* node = (CGruntzCommand*)g_singleCmdList.RemoveTail();
        if (node) {
            delete node;
        }
    }
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::FreeAll() - 0x024490. Drain the per-class recycle list:
// while it is non-empty, RemoveTail() a node and `delete` it (the virtual
// scalar-deleting dtor with flag 1). A static method (no this).
// ---------------------------------------------------------------------------
RVA(0x00024490, 0x29)
void CGruntzMultiCommand::FreeAll() {
    while (g_multiCmdCount) {
        CGruntzCommand* node = (CGruntzCommand*)g_multiCmdList.RemoveTail();
        if (node) {
            delete node;
        }
    }
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Serialize() - 0x0244d0 (vtable slot 1). The (de)serialize
// dispatcher: bail on a null stream; on mode 4 drive Save (slot 2), on mode 7
// drive Load (slot 3), both through the vtable; return 1 (or the sub-call's 0 on
// gate failure). Multi's identical twin is 0x0246c0.
// ---------------------------------------------------------------------------
RVA(0x000244d0, 0x3b)
i32 CGruntzSingleCommand::Serialize(CSerialArchive* s, i32 mode, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Save() - 0x024520 (vtable slot 2). Network-serialize the 8
// scalar fields out through the stream's Write (vtable slot +0x30). No-op (return 0)
// unless the stream is non-null AND the registry's active-game gate (g_gameReg->
// m_world) is set. The base declares Save pure; this is the single-target override.
// ---------------------------------------------------------------------------
RVA(0x00024520, 0x98)
i32 CGruntzSingleCommand::Save(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Write(&m_4, 1);
    s->Write(&m_5, 1);
    s->Write(&m_6, 1);
    s->Write(&m_8, 2);
    s->Write(&m_a, 2);
    s->Write(&m_submitted, 4);
    s->Write(&m_10, 1);
    s->Write(&m_11, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Load() - 0x0245f0 (vtable slot 3). The read twin: same
// gate, the 8 scalar fields read back through the stream's Read (vtable slot +0x2c).
// ---------------------------------------------------------------------------
RVA(0x000245f0, 0x98)
i32 CGruntzSingleCommand::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Read(&m_4, 1);
    s->Read(&m_5, 1);
    s->Read(&m_6, 1);
    s->Read(&m_8, 2);
    s->Read(&m_a, 2);
    s->Read(&m_submitted, 4);
    s->Read(&m_10, 1);
    s->Read(&m_11, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Serialize() - 0x0246c0 (vtable slot 1). The multi-target
// twin of CGruntzSingleCommand::Serialize: identical dispatcher body, but drives
// this class's own overridden Save/Load (slots 2/3).
// ---------------------------------------------------------------------------
RVA(0x000246c0, 0x3b)
i32 CGruntzMultiCommand::Serialize(CSerialArchive* s, i32 mode, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Save() - 0x024710 (vtable slot 2). The multi-target write
// twin: same gate/shape as the base Save, but writes the +0x10 field as one
// 16-bit value (the multi-command's flag mask) instead of the m_10/m_11 byte pair.
// ---------------------------------------------------------------------------
RVA(0x00024710, 0x8b)
i32 CGruntzMultiCommand::Save(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Write(&m_4, 1);
    s->Write(&m_5, 1);
    s->Write(&m_6, 1);
    s->Write(&m_8, 2);
    s->Write(&m_a, 2);
    s->Write(&m_submitted, 4);
    s->Write(&m_10, 2);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Load() - 0x0247d0 (vtable slot 3). Same gate/shape as the
// base Load, but reads the +0x10 field as one 16-bit value (the multi-command's
// flag mask) instead of the m_10/m_11 byte pair.
// ---------------------------------------------------------------------------
RVA(0x000247d0, 0x8b)
i32 CGruntzMultiCommand::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Read(&m_4, 1);
    s->Read(&m_5, 1);
    s->Read(&m_6, 1);
    s->Read(&m_8, 2);
    s->Read(&m_a, 2);
    s->Read(&m_submitted, 4);
    s->Read(&m_10, 2);
    return 1;
}

// size 0x14 from operator-new vtable attribution (gruntz.analysis.news)
// size 0x14 from operator-new vtable attribution (gruntz.analysis.news)

// The 1<<i bit table (0x5e9608) the mask builder/scanner indexes. DATA-pinned so
// the *(short*)... mask loop's address operands reloc-mask against it.
DATA(0x001e9608)
const u16 g_cmdBitTable[16] = {
    1,
    2,
    4,
    8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000
};

// ---------------------------------------------------------------------------
// The numeric settings DialogProc (0x092ab0), re-homed from the ApiCaller stubs:
// CGruntzCommand::ApplyOne / ApplyMask (0x024140/0x024190) drive the command
// dialog it backs. WM_INITDIALOG loads the 12 cached numeric edit fields into
// controls 0x4db..0x4e6; IDOK reads them back; IDCANCEL closes. The 12 field
// caches are cluster-local globals (moved here with the DlgProc).
// ---------------------------------------------------------------------------
DATA(0x0024526c)
extern i32 g_dlgVal_64526c;
DATA(0x002452d0)
extern i32 g_dlgVal_6452d0;
DATA(0x00245268)
extern i32 g_dlgVal_645268;
DATA(0x00245568)
extern i32 g_dlgVal_645568;
DATA(0x00245538)
extern i32 g_dlgVal_645538;
DATA(0x002451a4)
extern i32 g_dlgVal_6451a4;
DATA(0x002452d4)
extern i32 g_dlgVal_6452d4;
DATA(0x002452a8)
extern i32 g_dlgVal_6452a8;
DATA(0x00245558)
extern i32 g_dlgVal_645558;
DATA(0x00245560)
extern i32 g_dlgVal_645560;
DATA(0x0024555c)
extern i32 g_dlgVal_64555c;
DATA(0x00245564)
extern i32 g_dlgVal_645564;
RVA(0x00092ab0, 0x20d)
i32 CALLBACK winapi_092ab0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            SetDlgItemInt(hDlg, 0x4db, g_dlgVal_64526c, 0);
            SetDlgItemInt(hDlg, 0x4da, g_dlgVal_6452d0, 0);
            SetDlgItemInt(hDlg, 0x4dc, g_dlgVal_645268, 0);
            SetDlgItemInt(hDlg, 0x4dd, g_dlgVal_645568, 0);
            SetDlgItemInt(hDlg, 0x4de, g_dlgVal_645538, 0);
            SetDlgItemInt(hDlg, 0x4df, g_dlgVal_6451a4, 0);
            SetDlgItemInt(hDlg, 0x4e0, g_dlgVal_6452d4, 0);
            SetDlgItemInt(hDlg, 0x4e9, g_dlgVal_6452a8, 0);
            SetDlgItemInt(hDlg, 0x4e3, g_dlgVal_645558, 0);
            SetDlgItemInt(hDlg, 0x4e4, g_dlgVal_645560, 0);
            SetDlgItemInt(hDlg, 0x4e5, g_dlgVal_64555c, 0);
            SetDlgItemInt(hDlg, 0x4e6, g_dlgVal_645564, 0);
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                g_dlgVal_64526c = GetDlgItemInt(hDlg, 0x4db, 0, 0);
                g_dlgVal_6452d0 = GetDlgItemInt(hDlg, 0x4da, 0, 0);
                g_dlgVal_645268 = GetDlgItemInt(hDlg, 0x4dc, 0, 0);
                g_dlgVal_645568 = GetDlgItemInt(hDlg, 0x4dd, 0, 0);
                g_dlgVal_645538 = GetDlgItemInt(hDlg, 0x4de, 0, 0);
                g_dlgVal_6451a4 = GetDlgItemInt(hDlg, 0x4df, 0, 0);
                g_dlgVal_6452d4 = GetDlgItemInt(hDlg, 0x4e0, 0, 0);
                g_dlgVal_6452a8 = GetDlgItemInt(hDlg, 0x4e9, 0, 0);
                g_dlgVal_645558 = GetDlgItemInt(hDlg, 0x4e3, 0, 0);
                g_dlgVal_645560 = GetDlgItemInt(hDlg, 0x4e4, 0, 0);
                g_dlgVal_64555c = GetDlgItemInt(hDlg, 0x4e5, 0, 0);
                g_dlgVal_645564 = GetDlgItemInt(hDlg, 0x4e6, 0, 0);
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}
