#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

class CDDrawWorker;             // CDDrawWorker IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);

#include <Mfc.h> // real MFC CString (the WwdFile members take it by value)
#include <Ints.h>
#include <rva.h>

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

struct WwdHeader {
    u32 wwdSignature; // +0x000  == 1524 (header size)
    u32 field_4;      // +0x004
    u32 flags;        // +0x008  bit0 (0x1) USE_Z_COORDS, bit1 (0x2) COMPRESS
    u8 pad_c[0x10 - 0x0c];
    char levelName[0x2d0 - 0x10]; // +0x010  (name/author/paths block)
    i32 startX;                   // +0x2D0
    i32 startY;                   // +0x2D4
    u32 pad_2d8;                  // +0x2D8
    u32 numPlanes;                // +0x2DC
    u32 planesOffset;             // +0x2E0
    u32 tileDescriptionsOffset;   // +0x2E4
    u32 mainBlockLength;          // +0x2E8  inflated main-block size
    u32 checksum;                 // +0x2EC
    u8 pad_2f0[0x5f4 - 0x2f0];
};
SIZE(0x5f4); // on-disk WWD header (RE'd 0x5F4 bytes)

class WwdInputStream {
public:
    WwdInputStream();
    ~WwdInputStream();
    i32 Open(const char* name, i32 mode, void* errSink);
    i32 Read(void* buf, i32 len);

private:
    // +0x00 is the engine vtable pointer of this EXTERNAL binary-file-stream class
    // (its ctor/dtor/Open/Read are all unmatched engine code, reloc-masked). Our
    // code never touches this slot; it is a pure layout placeholder so m_handle
    // lands at +0x04. Modeling it as C++ virtuals would make cl emit a WRONG,
    // incomplete vtable in this TU (its real virtuals are unmodeled) and regress
    // the neighbouring Save -> kept as an opaque leading word.
    // authentic: opaque foreign slots - m_00 is the EXTERNAL stream class's engine
    // vptr (its virtuals are unmatched; declaring them would emit a wrong ??_7 and
    // regress Save), m_handle is a Win32 HANDLE, m_name the engine CString buffer.
    // None are dereferenced here; they are pure layout placeholders. Kept void*.
    char _vft0[4];   // +0x00  engine vptr (reduced view; not dispatched)
    HANDLE m_handle; // +0x04  Win32 HANDLE (-1 when closed); never dereferenced here
    i32 m_open;      // +0x08  open/refcount flag
    char* m_name;    // +0x0C  CString filename buffer (the CString body pointer)
};
SIZE(0x10); // 16-byte file-stream object (full layout to +0xc)

#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost (+ CDDrawWorkerHost/CDDrawWorkerHost/

class CDDSurface;
struct CPlaneTile;

struct CPlaneFrame {
    u8 pad_0[0x14];
    CPlaneTile** m_frames; // +0x14  frame table (indexed by the low 16 bits of handle)
    u8 pad_18[0x64 - 0x18];
    i32 m_lo; // +0x64  valid handle range [m_lo, m_hi]
    i32 m_hi; // +0x68
};
SIZE_UNKNOWN();

struct CPlaneTile {
    u8 pad_0[0x28];
    u32 m_trans;       // +0x28  BltFast transparency/colour-key flag
    CDDSurface* m_src; // +0x2c  source surface
};
SIZE_UNKNOWN();

struct CPlaneDrawCtx {
    u8 pad_0[0x2c];
    CDDSurface* m_surface; // +0x2c  the blit target surface
};
SIZE_UNKNOWN();

// The palette tail hanging off CDDrawWorkerMapSmall::m_cachedWorker (+0x64).
// @identity-TODO: these two are the ONLY links of the cascade still unproven - the
// cached worker is a CObject* out of the map (RemoveByValue proves that much), but
// WHICH palette-bearing worker class it is, no caller/new-site the xref DB can reach
// names (its only reader is the plane's ResolveColorKey). Kept as honest named
// records rather than a fabricated identity; the RGB888 table at +0xc is what is
// load-bearing.
//
// LEAD for whoever closes it (do not treat as proof): CDDrawWorkerMapSmall's own
// methods treat every m_map1 value as a CAniRecordBase2 (DestroyAll `delete
// ((CAniRecordBase2*)val)`, RemoveByKey's cast), and +0x64 caches a map value - so
// the cached worker is very likely a CAniRecordBase2, whose +0x10 "owned work
// buffer" would then BE this palette array. What is missing is a byte that ties
// that generic work buffer to the RGB888 layout; until one turns up this stays a
// lead, and the slot keeps the map's own element type (CObject*).
struct CPlanePalArr {
    u8 pad_0[0xc];
    u8* m_rgb; // +0xc  RGB888 triples (4 bytes/entry)
};
SIZE_UNKNOWN();
struct CPlanePalOwner {
    u8 pad_0[0x10];
    CPlanePalArr* m_palette; // +0x10
};
SIZE_UNKNOWN();
class CFileMemBase;

extern void* operator new(u32 size);
extern void operator delete(void* p);

extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

extern "C" i32 __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

i32 __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

class WwdFile {
public:
    static i32 ValidateMainBlock(CString name);
    // GetMapBaseName (0x3bb50, static __cdecl, returns CString by value): given a
    // map file path, return the filename portion (after the last '\\') with the
    // 4-char extension dropped, via the shared 0x62c010 scratch buffer. Empty or
    // <= 4-char paths come back unchanged.
    static CString GetMapBaseName(CString path);
    // (RebuildPlanes @0x1628f0 + ReadPlaneObjects @0x162af0 are GONE from here: their
    // `this` IS the plane - they read m_mapData@+0x0c, m_wrapW/m_wrapH@+0x30/+0x34 and
    // the spatial worker @+0xb0 off it - so they are CDDrawWorkerHost methods now. This
    // class keeps only its two genuinely-static WWD helpers.)
};
SIZE_UNKNOWN(); // namespace-class (method-only)

#endif // SRC_WWD_WWDFILE_H
