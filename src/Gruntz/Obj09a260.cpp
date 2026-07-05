// Obj09a260.cpp (renamed from ClassUnknown30.cpp) - Obj09a260, a spawn-list entry
// whose leading member is a CString name. XREF: GetStr (0x9a260) is called on entries
// returned by CSpawnList::FindEntry/Extract (areamgr, returning CSpawnEntryN*),
// CObjResTree::LoadObject{Image,Sound,Anim}Resources (loadobjectresources) and
// CGruntSpawnConfig::PickWeighted (gruntspawnconfig) - i.e. it is the by-value name
// getter of the spawn-entry record.
//
// IDENTITY CONFIRMED (matcher-5): Obj09a260 == CSpawnEntryN. The canonical class
// already exists as a .cpp-local view in src/Gruntz/AreaMgr.cpp (`class CSpawnEntryN
// { CString GetName(); // 0x2a1d };`), and GetName's thunk 0x2a1d jmps to THIS body
// at 0x9a260 - so GetStr_09a260 IS CSpawnEntryN::GetName's out-of-line body.
// DEFERRED FOLD (not done here to avoid regressing the matched areamgr unit + a name
// hazard): the clean fold is to hoist CSpawnEntryN/CSpawnNode/CSpawnList out of
// AreaMgr.cpp into a shared header and reconstruct this as CSpawnEntryN::GetName.
// CAVEAT: AreaMgr.cpp ALSO defines a DIFFERENT `class CSpawnEntry` (name record,
// GetTail@0x9a830) and GruntSpawnConfig.h a THIRD `struct CSpawnEntry` (voice list,
// CObList@+0) - resolve that CSpawnEntry name collision first (real distinct classes,
// not one). RTTI name still a Ghidra placeholder ("Obj09a260").
//
// Obj09a260::GetStr (0x09a260, __thiscall) returns the
// leading CString member by value (NRVO into the hidden return slot): a single
// CString copy-ctor (0x1b9ba3) from m_str into *retptr. The MFC copy-ctor is
// external (reloc-masked).
#include <rva.h>
#include <Mfc.h>

SIZE_UNKNOWN(Obj09a260);
class Obj09a260 {
public:
    CString m_str; // offset 0x0
    CString GetStr_09a260();
};

// @early-stop
// 66.7%: returns the leading CString member by value (single copy-ctor 0x1b9ba3
// into the NRVO return slot) - logic correct. Retail reserves+zeroes one dead
// 4-byte stack local (push ecx; mov [esp+8],0; pop ecx) that no return-by-value
// spelling reproduces under MSVC5 /O2 (DCE'd here); unreproduced codegen artifact.
RVA(0x0009a260, 0x1d)
CString Obj09a260::GetStr_09a260() {
    return m_str;
}
