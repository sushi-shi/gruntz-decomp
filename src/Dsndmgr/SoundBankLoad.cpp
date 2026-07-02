// SoundBankLoad.cpp - the named-resource loader of a WAP32 sound-bank object (the
// DSoundList/CGruntzSoundZ neighbourhood; engine_boundary 0x138aa0). The method
// `Load(name, arg)` either forwards to the slot-0x3c handler when the name is the
// special ".." token, or opens `name` as a file, slurps it whole into the owned
// +0x5c buffer (operator new), and hands it to the slot-0x14 decode virtual. The
// local CFile (a throwing ctor) forces the /GX EH frame. Field names are
// placeholders; only the OFFSETS + the emitted bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// The engine throwing allocator (global operator new @0x1b9b46, NAFXCW). Reloc-masked.
void* operator new(u32 n);

// The name compare against the pooled ".." token (0x120090, __cdecl 2-arg; strcmp/
// _mbscmp). Reloc-masked rel32; the ".." is the shared $SG constant (0x5ee8ec) the
// ButeMgr parser also references, so reach it by symbol so the DIR32 pairs.
extern "C" i32 SbNameCmp(const char* a, const char* b); // 0x120090
DATA(0x001ee8ec)
extern char g_dotDot[]; // 0x5ee8ec  ".."

// The stack CFile temp: ctor/dtor/Open/GetLength/Read act on the object base (the
// `lea ecx,[file]` shape), all reloc-masked NAFXCW members.
struct RezFile {
    RezFile();                                       // 0x1befd7
    ~RezFile();                                      // 0x1bf121
    i32 Open(const char* path, i32 mode, i32 share); // 0x1bf200
    i32 GetLength();                                 // 0x1bf505
    i32 Read(void* buf, i32 len);                    // 0x1bf328
};
SIZE_UNKNOWN(RezFile); // CFile-temp view (NAFXCW members reloc-masked)

// The sound-bank object. Polymorphic with a 0x14-slot vtable: slot 5 (+0x14) is the
// in-memory decode, slot 15 (+0x3c) the special-name handler. The virtuals are
// declared (not defined here) so the class never emits a vtable in this TU; the calls
// lower to `mov eax,[this]; call [eax+N]`. The owned load buffer lives at +0x5c.
struct CSoundBank {
    virtual void s00();                               // +0x00
    virtual void s01();                               // +0x04
    virtual void s02();                               // +0x08
    virtual void s03();                               // +0x0c
    virtual void s04();                               // +0x10
    virtual i32 DecodeBuf(void* buf, i32 len, i32 a); // slot 5  (+0x14)
    virtual void s06();                               // +0x18
    virtual void s07();                               // +0x1c
    virtual void s08();                               // +0x20
    virtual void s09();                               // +0x24
    virtual void s0a();                               // +0x28
    virtual void s0b();                               // +0x2c
    virtual void s0c();                               // +0x30
    virtual void s0d();                               // +0x34
    virtual void s0e();                               // +0x38
    virtual i32 LoadSpecial(const char* path, i32 a); // slot 15 (+0x3c)

    char m_pad04[0x5c - 0x04];
    void* m_loadBuffer; // +0x5c  owned load buffer

    i32 Load(const char* path, i32 decodeArg); // 0x138aa0
};
SIZE_UNKNOWN(CSoundBank); // polymorphic; only +0x5c pinned, tail opaque

// Load: the special ".." name forwards to the slot-0x3c handler; otherwise
// open `path`, require >= 4 bytes, slurp it whole into the owned +0x5c buffer, and run
// the slot-0x14 decode. Each failure tears down the local CFile and returns 0; the
// CFile's throwing ctor forces the /GX frame. __thiscall, ret 8.
//
// @early-stop
// /GX EH-frame + reloc-name wall: the ".."-token branch, the CFile open/GetLength/new/
// Read/decode chain, the slot-0x14/0x3c vtable dispatches and the m_5c store are byte-
// faithful, but the local CFile's stack-slot scheduling inside the /GX frame + the
// EH-state numbering (the documented eh-scoped-local wall) and the differently-named
// SbNameCmp/operator-new reloc operands diverge. Logic complete; final sweep.
RVA(0x00138aa0, 0x175)
i32 CSoundBank::Load(const char* path, i32 decodeArg) {
    if (SbNameCmp(path, g_dotDot) != 0) {
        RezFile file;
        if (!file.Open(path, 0, 0)) {
            return 0;
        }
        i32 fileLength = file.GetLength();
        if (fileLength < 4) {
            return 0;
        }
        m_loadBuffer = operator new(fileLength);
        if (m_loadBuffer == 0) {
            return 0;
        }
        if (file.Read(m_loadBuffer, fileLength) != fileLength) {
            return 0;
        }
        return DecodeBuf(m_loadBuffer, fileLength, decodeArg);
    }
    return LoadSpecial(path, decodeArg);
}
