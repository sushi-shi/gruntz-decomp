// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// On-disk WWD header layout is pinned in src/Stub/types/wwd.h; this header
// reproduces the load-bearing fields the loaders actually touch.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

typedef unsigned char Bytef;
typedef unsigned long uLong;
typedef unsigned long uLongf;

// WWD file header (1524 = 0x5F4 bytes on disk). Only the fields the loaders
// read are named; the rest is padding to preserve offsets. See formats/wwd.h.
struct WwdHeader {
    unsigned int wwdSignature; // +0x000  == 1524 (header size)
    unsigned int field_4;      // +0x004
    unsigned int flags;        // +0x008  bit1 (0x2) = COMPRESS
    unsigned char pad_c[0x10 - 0x0c];
    char levelName[0x2d0 - 0x10];        // +0x010  (name/author/paths block)
    int startX;                          // +0x2D0
    int startY;                          // +0x2D4
    unsigned int pad_2d8;                // +0x2D8
    unsigned int numPlanes;              // +0x2DC
    unsigned int planesOffset;           // +0x2E0
    unsigned int tileDescriptionsOffset; // +0x2E4
    unsigned int mainBlockLength;        // +0x2E8  inflated main-block size
    unsigned int checksum;               // +0x2EC
    unsigned char pad_2f0[0x5f4 - 0x2f0];
};

// ---------------------------------------------------------------------------
// Engine binary file stream (16 bytes). The validators construct one on the
// stack, Open() a file, Read() the header, then let the destructor (Close) run.
// All four members are UNMATCHED engine functions (ctor, dtor, Open, Read);
// their call bytes are reloc-masked in objdiff.
// Layout: vtable@+0, HANDLE@+4, openFlag@+8, CString filename@+0xc.
//   - ctor sets up the vtable + CString and leaves HANDLE = -1.
//   - Open(name, mode, errSink) opens via CreateFileA; returns nonzero on
//     success, 0 on failure (the engine's reversed sense).
//   - Read(buf, len) ReadFiles len bytes into buf; returns the count read.
//   - the destructor CloseHandle()s an open handle (HANDLE != -1).
// ---------------------------------------------------------------------------
class WwdInputStream {
public:
    WwdInputStream();
    ~WwdInputStream();
    int Open(const char* name, int mode, void* errSink);
    int Read(void* buf, int len);

private:
    void* m_vtbl;   // +0x00
    void* m_handle; // +0x04  HANDLE (-1 when closed)
    int m_open;     // +0x08  open/refcount flag
    void* m_name;   // +0x0C  CString filename buffer
};

// ---------------------------------------------------------------------------
// CPlane - the in-memory plane object ReadPlane constructs (one per WWD plane).
// UNMATCHED engine class; modeled as an external shell so ReadPlane's calls to
// it reloc-mask. Size 0x158 (344 B); ctor with three
// args, two-phase vtable. ReadPlane uses
// only the vtable: slot 0xA (offset 0x28) = the plane-block reader (reads one
// WwdPlaneHeader at 0xA0 stride + fans out to tile/imageset/object sub-readers);
// slot 0x1 (offset 0x4) = the scalar-deleting destructor. m_flags@+0x8 carries
// the plane flags (bit0 = MAIN). None of these have bodies here.
// ---------------------------------------------------------------------------
class CPlane {
public:
    // Construct from (a, planeIndex, c); reloc-masked engine ctor.
    CPlane(void* a, int planeIndex, void* c);
    // vtable: +0x28 reader(planeData, blockBase, outCtx) -> nonzero on success.
    virtual int dummy0();
    virtual void dtor(int flags);                                     // +0x04  scalar-deleting dtor
    virtual int dummy2();                                             // +0x08
    virtual int dummy3();                                             // +0x0c
    virtual int dummy4();                                             // +0x10
    virtual int dummy5();                                             // +0x14
    virtual int dummy6();                                             // +0x18
    virtual int dummy7();                                             // +0x1c
    virtual int dummy8();                                             // +0x20
    virtual int dummy9();                                             // +0x24
    virtual int Read(void* planeData, void* blockBase, void* outCtx); // +0x28

    unsigned char pad_4[0x4]; // +0x04
    unsigned int m_flags;     // +0x08  bit0 = MAIN/origin-fixed plane
    unsigned char pad_c[0x10 - 0x0c];
    float m_scaledX; // +0x10  startX (or startX * m_scaleX)
    float m_scaledY; // +0x14  startY (or startY * m_scaleY)
    float m_scaleX;  // +0x18  X parallax factor
    float m_scaleY;  // +0x1c  Y parallax factor
    unsigned char pad_20[0x84 - 0x20];
    int m_originX;                      // +0x84
    int m_originY;                      // +0x88
    unsigned char pad_8c[0x158 - 0x8c]; // pad to the real CPlane size (0x158)
};

// ---------------------------------------------------------------------------
// CGameLevel - the level-load orchestrator (a.k.a. CDDrawLevelData). ReadPlane is a
// __thiscall member on it. Only the members ReadPlane touches are pinned here
// (the rest of the class is reconstructed in src/Gruntz/GameLevel.{cpp,h}):
//   +0x0C m_field0c   - 1st CPlane ctor arg (the owning context the planes share)
//   +0x10 m_planeCtx  - &this->m_planeCtx is the 3rd arg to CPlane::Read
//   +0x34 m_planes    - MFC CArray<CPlane*> (SetAtGrow grows + stores)
//   +0x3C m_planeCount- current plane index / count (the running loop counter)
//   +0x5C m_mainPlane - cached pointer to the MAIN plane
//   +0x60 m_mainIndex - index of the MAIN plane (m_planeCount - 1 at capture)
// ---------------------------------------------------------------------------
class CGameLevelPlanes {
public:
    // ReadPlane: new CPlane; if its block reader
    // succeeds, append it to m_planes and record the MAIN plane; else delete it.
    CPlane* ReadPlane(void* planeData, void* blockBase, void* unused);

    unsigned char pad_0[0x0c];
    void* m_field0c;                       // +0x0C
    unsigned char m_planeCtx[0x34 - 0x10]; // +0x10  (&m_planeCtx -> CPlane::Read)
    unsigned char m_planes[0x3c - 0x34];   // +0x34  CArray<CPlane*>
    int m_planeCount;                      // +0x3C
    unsigned char pad_40[0x5c - 0x40];
    CPlane* m_mainPlane; // +0x5C
    int m_mainIndex;     // +0x60
};

// MFC CArray<CPlane*>::SetAtGrow(index, value).
// Grows the backing store to fit `index` then stores value at [index].
struct CPlanePtrArray {
    void SetAtGrow(int index, CPlane* value);
};

// Global operator new (NAFXCW new-handler loop) / delete.
extern void* operator new(unsigned int size);
extern void operator delete(void* p);

// zlib one-shot decompressor (matched, vendor/zlib-1.0.4/uncompr.c).
extern "C" int uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

// Inflate the WWD main block (planes/tiles) in place into a caller buffer.
// __stdcall free function (callee cleans 12 bytes).
extern "C" int __stdcall
WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, unsigned int destLen);

// Validate a WWD file by name: open it, read the 1524-byte header into the
// caller buffer, require read == 0x5F4 and header signature <= 0x5F4.
// __stdcall free function (callee cleans 8 bytes); returns 1 / 0.
int __stdcall WwdFile_IsValidWwd(const char* name, void* headerBuf);

// Validate a WWD file by name into a private 1524-byte buffer, then copy that
// header into the caller buffer. __stdcall (callee cleans 8 bytes); returns 1 / 0.
int __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

#endif // SRC_WWD_WWDFILE_H
