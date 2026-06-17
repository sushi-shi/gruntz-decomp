// UnknownMinerva.cpp - leaf methods of the tomalla-named class UnknownMinerva
// (a CDirectDrawMgr surface/page sub-manager in the "Harry Potter" family).
// ClearUnknownMap is the map-teardown counterpart to the factory methods on
// other Unknown* classes: iterates the CMapStringToOb at +0x10, running each
// entry's scalar-deleting destructor, then calls RemoveAll. The local CString
// key with a destructor forces a C++ EH frame (fs:0 / __except_handler3),
// so this TU is built with /GX.
//
// VirtualMethodUnknown14 checks the two fields at +0x2c and +0x30 (post-map
// record) for readiness. Plain /O2 /MT leaf: no SEH, no relocations.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// MFC placeholders - only the call symbols + the 0x10 map offset matter
// ---------------------------------------------------------------------------
class CObject;

class CString {
public:
    CString();                      // @0x1b9b93
    ~CString();                     // @0x1b9cde
    char *m_pchData;                // +0x00
};

struct __POSITION { };

// CMapStringToOb at UnknownMinerva+0x10. GetNextAssoc/RemoveAll are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC
// signatures so clang mangles them to ?GetNextAssoc@CMapStringToOb@@... /
// ?RemoveAll@CMapStringToOb@@... .
class CMapStringToOb {
public:
    void GetNextAssoc(__POSITION *&rNextPosition, CString &rKey,
                      CObject *&rValue) const;             // @0x1b8546
    void RemoveAll();                                       // @0x1b82d0
};

// The map-value child interface: only the scalar-deleting destructor slot
// (+0x04) is load-bearing. Declarations only - never defined, so no ??_7
// is emitted here.
class MinervaValue {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04  scalar-deleting destructor
};

// ---------------------------------------------------------------------------
// UnknownMinerva - the CMapStringToOb at +0x10 extends 28 bytes to +0x2c.
// The m_1c member is the map's internal m_nCount field (at map+0x0c).
// The two readiness fields are at +0x2c and +0x30 (post-map record).
// ---------------------------------------------------------------------------
class UnknownMinerva {
public:
    int  VirtualMethodUnknown14();
    void ClearUnknownMap();

    void          *m_vptr;                  // +0x00
    int            m_04;                    // +0x04  -1 when inactive
    char           m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int            m_0c;                    // +0x0c  parent/root handle
    CMapStringToOb m_10;                    // +0x10  map (28 B, to +0x2c)
    int            m_2c;                    // +0x2c  readiness field 1
    int            m_30;                    // +0x30  readiness field 2
};

// Read the CMapStringToOb internal m_nCount at +0x1c from the class base.
// (map+0x0c = +0x1c from UnknownMinerva start)
static inline int MinervaMapCount(const UnknownMinerva *p)
{
    return *(const int *)((const char *)p + 0x1c);
}

// ---------------------------------------------------------------------------
// UnknownMinerva::VirtualMethodUnknown14  @0x157530  (__thiscall, ret 0)
// Ready when either post-map field is non-zero.
// ---------------------------------------------------------------------------
// @address: 0x157530
// @size:    0x17
int UnknownMinerva::VirtualMethodUnknown14()
{
    if (m_2c != 0)
        return 1;
    if (m_30 != 0)
        return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownMinerva::ClearUnknownMap  @0x157bc0  (__thiscall, ret 0)
// Destructs each non-null map value via its scalar-deleting dtor, then calls
// RemoveAll. The local CString key with a destructor forces a C++ EH frame
// (fs:0 / __except_handler3), so this TU needs /GX.
//
// The map's internal m_nCount is read and converted to -1/0 (the standard
// neg/sbb idiom) which serves double duty as both the loop guard and the
// initial POSITION sentinel for GetNextAssoc (the NAFXCW build interprets
// -1 as "start from the first bucket," avoiding a separate GetStartPosition).
// ---------------------------------------------------------------------------
// @address: 0x157bc0
// @size:    0xa2
void UnknownMinerva::ClearUnknownMap()
{
    int flag = MinervaMapCount(this);
    flag = -flag;

    CString key;

    if (flag) {
        CObject *val;
        __POSITION *pos = (__POSITION *)-1;
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0)
                ((MinervaValue *)val)->ScalarDtor(1);
        } while (pos != 0);
    }

    m_10.RemoveAll();
}
