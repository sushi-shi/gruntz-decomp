// UnknownSeverus.cpp - leaf method(s) of the tomalla-named ddrawmgr surface-family
// sub-manager UnknownSeverus (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// UnknownSeverus owns a CMapStringToOb at +0x10 (m_unknownMap) keyed by const char*
// strings. VirtualMethodUnknown54 is a keyed remove-and-destroy: Lookup the key in
// the map; if present, RemoveKey it and run the found value's scalar-deleting
// destructor (vtable +0x4, arg 1). Plain /O2 /MT leaf: NO SEH frame, NO data
// relocations - the only relocations are the reloc-masked rel32 thunk calls
// (CMapStringToOb::Lookup / RemoveKey, both out-of-line NAFXCW), and the found
// value's destructor is dispatched through its own vtable.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine). The looked-up value is modeled as a polymorphic
// stub (virtuals at the right slots) ONLY so `val->ScalarDtor(1)` lowers to the
// exact `mov eax,[val]; push 1; call [eax+4]` __thiscall dispatch; its virtuals are
// never defined, so no vtable is emitted in this TU.
//
// SIBLING DEFERRED: UnknownSeverus::VirtualMethodUnknown58 @0x165210 (162 B) is the
// map-teardown counterpart; it carries a C++ EH frame (a stack CString iteration
// key with a destructor) and a GetNextAssoc loop with a per-iteration stack-layout
// shift. Deferred to its own pass to keep this TU /O2 /MT and free of /GX entropy.
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols + the 0x10 map offset matter) -----
class CObject;

// The looked-up value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing. Declarations only - never defined, so no ??_7 is emitted here.
class SeverusValue {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04  scalar-deleting destructor
};

// CMapStringToOb lives at UnknownSeverus+0x10. Lookup/RemoveKey are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC signatures
// so clang mangles them to ?Lookup@CMapStringToOb@@... / ?RemoveKey@CMapStringToOb@@... .
class CMapStringToOb {
public:
    int Lookup(const char *key, CObject *&rValue) const;    // @0x1b8008
    int RemoveKey(const char *key);                         // @0x1b80ae
};

// ---------------------------------------------------------------------------
// UnknownSeverus - only the load-bearing offset is modeled: the CMapStringToOb at
// +0x10. The matched method occupies a lower vtable slot (slot number not load-
// bearing, only body), placed last.
// ---------------------------------------------------------------------------
class UnknownSeverus {
public:
    void VirtualMethodUnknown54(const char *key);

    void          *m_vptr;                  // +0x00
    char           m_pad04[0x10 - 0x04];    // +0x04..0x0f
    CMapStringToOb m_10;                     // +0x10  m_unknownMap
};

// ---------------------------------------------------------------------------
// UnknownSeverus::VirtualMethodUnknown54  @0x156ec0  (__thiscall, ret 0x4)
// Lookup `key` in the map; if found, RemoveKey it and run the value's scalar-
// deleting destructor (vtbl +0x4, arg 1).
//
// RESIDUE (~77.5%, NOT a logic/offset/type/CFG error - register-allocation +
// store/load-scheduling entropy, matching-patterns.md / match-learnings.md
// "register-alloc coin-flip"): the logic, CFG, the val=0 init, both library calls,
// their args, and the dtor dispatch are all reproduced; only the register schedule
// the allocator picked differs. The target holds `key` in EDI and keeps `val`
// purely on the stack, reloading it (`mov ecx,[esp+8]`) AFTER RemoveKey and
// branching on Lookup's return in EAX. MSVC5 on this (isolated) TU instead caches
// `key` in EBX and `val` in EDI across the calls and branches on the cached val -
// equivalent codegen, an extra callee-saved register, a different ecx/edx pairing.
// Every source form tried (if(Lookup(...)) / if(...!=0) / captured `int found` /
// volatile val) produced the identical schedule; the surrounding symbol-set is what
// re-rolls the allocation, so no in-function lever flips it. Left as the plateau.
// ---------------------------------------------------------------------------
// @address: 0x156ec0
// @size:    0x40
void UnknownSeverus::VirtualMethodUnknown54(const char *key)
{
    CObject *val = 0;
    if (m_10.Lookup(key, val)) {
        m_10.RemoveKey(key);
        ((SeverusValue *)val)->ScalarDtor(1);
    }
}
