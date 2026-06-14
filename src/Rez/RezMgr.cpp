// RezMgr.cpp - the Monolith "RezMgr" archive container classes (CRezItm leaf /
// CRezDir subdirectory nodes) and the directory walk over a Gruntz.REZ /
// GRUNTZ.VRZ archive.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CRezItm::CRezItm(parent)        @ 0x13c540  (40 B,  thiscall ret 4)  BYTE-EXACT  - leaf ctor (new 0x24)
//   CRezDir::CRezDir(parent,rezmgr) @ 0x13c940  (70 B,  thiscall ret 8)  PLATEAU 78% - dir  ctor (new 0x38)
//   CRezDir::FindEntry(name)        @ 0x13c080  (60 B,  thiscall ret 4)  BYTE-EXACT  - is-this-a-dir? stat
//   CRezDirNode::Load(childFlag)    @ 0x13a0f0  (153 B, thiscall ret 4)  BYTE-EXACT  - recursive dir parse
//   RezMgr::MakeImageKey(...)       @ 0x13e5d0  (177 B, thiscall ret 0xc) BYTE-EXACT  - ext-dispatch image load (.BMP/.PCX/.PID)
//   RezMgr::MakeRezPath()           @ 0x091670  (684 B, thiscall ret)    PLATEAU 92% - archive-path builder (EH/CString entropy)
//
// Both ctors share the base ctor CRezItmBase::CRezItmBase @0x13c4e0 (stores the
// base vtable @0x5ef768 and the parent pointer @+0xc), then overwrite the vtable
// with the derived one (two-phase construction; all vtable stores reloc-masked).
// `operator new` sizes 0x24 (leaf) / 0x38 (dir) confirm the layouts. The
// "File is not sorted!" assert string is a reloc-masked file-scope literal.
//
// CRezDir ctor PLATEAU (78%): all 14 member stores go to the correct offsets
// with the correct values, but MSVC5 schedules the +0x10/+0x1c collection-vtable
// stores and the +0x14/+0x18 head/tail zeros in a different (still-correct) order
// than the target, and materializes the vtbl constant before the zero (vs after).
// No source lever flips it (tried 6 store orderings + an embedded collection
// sub-object - the sub-object emits an out-of-line ctor call, far worse). The
// vtable operands are reloc-masked. Entropy-class residue, left per the doctrine.
//
// OpenSub @0x13b0c0 (568 B) is NOT matched here: it runs on a THIRD, distinct
// node layout (it uses +0x1c as a child COUNT and +0x10 as a list-append target,
// directly conflicting with the 0x38 CRezDir ctor's vtable stores at those same
// offsets, AND with CRezDirNode's +0x10 size / +0x18 source - so all three
// "CRezDir"-labeled functions are actually three different classes). It also
// needs a faithful C++ EH frame, the inline CString strlen+strcpy, the embedded
// list-append helper, two-slot virtual dispatch on the allocated child, a
// 0xA8-byte item-header parse feeding running max-dims, and two large external
// tail calls (0x13b300 recursive FS walk, 0x13a580 item-record). >512 B of high
// EH/CString/virtual entropy; deferred to a dedicated worker per the prompt's
// "don't sacrifice a green fn" guidance. The container layouts it would confirm
// are already pinned by the two ctors below.
#include "RezMgr.h"

// ---------------------------------------------------------------------------
// CRezItmBase::CRezItmBase(parent)  @ 0x13c4e0 (18 B).
//   mov [this] = base vtbl (0x5ef768); mov [this+0xc] = parent.
// Out-of-line so the derived ctors emit a `call` to it.
// ---------------------------------------------------------------------------
CRezItmBase::CRezItmBase(void *parent)
{
    m_parent = parent;
}


// ---------------------------------------------------------------------------
// CRezItm::CRezItm(parent)  @ 0x13c540 (40 B, ret 4).
// Base ctor (vtbl @0x5ef768 + parent), then derived vtbl @0x5ef788, m_10 = 0,
// m_14 = 0, m_20 = -1. m_18/m_1c untouched.
// ---------------------------------------------------------------------------
CRezItm::CRezItm(void *parent) : CRezItmBase(parent)
{
    m_10 = 0;
    m_14 = 0;
    m_20 = -1;
}

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, rezMgr)  @ 0x13c940 (70 B, ret 8).
// Base ctor, then: m_14=0, m_18=0, m_vtblA=m_vtblB=0x5ef7c8 (embedded child
// collection's two vtables), m_20=m_24=m_28=m_34=0, derived vtbl @0x5ef7a8,
// m_2c=rezMgr, m_30=1.
// ---------------------------------------------------------------------------
CRezDir::CRezDir(void *parent, void *rezMgr) : CRezItmBase(parent)
{
    m_14 = 0;
    m_18 = 0;
    m_vtblA = (void *)0x5ef7c8;
    m_vtblB = (void *)0x5ef7c8;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_34 = 0;
    m_2c = rezMgr;
    m_30 = 1;
}

// ---------------------------------------------------------------------------
// CRezDir::FindEntry(char* name)  @ 0x13c080 (60 B, ret 4).
// Despite the tomalla "binary search" label, the bytes are a stat: build a
// 0x24-byte find-record on the stack, RezStatEntry(name, &rec); on failure
// return 0; on success return whether the entry's attribute dword (at byte +6
// of the record) has bit 0x4000 set (i.e. the entry is a directory).
// `this` is never read here.
// ---------------------------------------------------------------------------
int CRezDir::FindEntry(char *name)
{
    RezFindRec rec;
    if (RezStatEntry(name, &rec) != 0)
        return 0;
    return (*(int *)(rec.raw + 6) & 0x4000) == 0x4000;
}

// The "File is not sorted!" assert message - a file-scope literal (its address
// is the reloc-masked push operand in Load's failure path).
static const char s_notSorted[] = "CRezDir::Load Failed! (File is not sorted!)";

// ---------------------------------------------------------------------------
// CRezDirNode::Load(childFlag)  @ 0x13a0f0 (153 B, ret 4).
// Recursive directory parse / load. If already loaded (m_buf != 0) return 1.
// Validate the source (m_src->m_8 nonzero, m_src->m_1c <= 1) else assert "File
// is not sorted!". If m_size > 0, allocate the payload buffer and virtually read
// it from the source stream at (m_off, 0, m_size, buf). When childFlag is set,
// iterate the child collection (First/Next) and recurse Load(1) into each
// child's sub-dir node (node->m_14). Returns 1.
// ---------------------------------------------------------------------------
int CRezDirNode::Load(int childFlag)
{
    if (m_buf != 0)
        return 1;

    RezSrc *src = m_src;
    if (src->m_8 == 0 || (unsigned)src->m_1c > 1) {
        RezAssertFail(s_notSorted);
        return 0;
    }

    if (m_size > 0) {
        m_buf = RezAlloc(m_size);
        if (m_buf != 0)
            m_src->m_stream->ReadAt(m_off, 0, m_size, m_buf);
    }

    if (childFlag != 0) {
        for (RezNode *n = m_kids.First(); n != 0; n = n->Next())
            n->m_14->Load(1);
    }
    return 1;
}

// The image-resource extension keys (file-scope literals - their addresses are
// the reloc-masked push operands in MakeImageKey's stricmp calls). Order in the
// binary: .BMP @0x61a0e4, then .PCX @0x61a0dc, then .PID @0x61a0d4.
static const char s_extBmp[] = ".BMP";   // 0x61a0e4
static const char s_extPcx[] = ".PCX";   // 0x61a0dc
static const char s_extPid[] = ".PID";   // 0x61a0d4

// ---------------------------------------------------------------------------
// RezMgr::MakeImageKey(arg1, name, arg3)  @ 0x13e5d0 (177 B, thiscall ret 0xc).
// Dispatch a resource load by the file extension of `name`: locate the last
// '.', then case-insensitively match .BMP/.PCX/.PID and hand off to the
// matching loader (LoadBmp/LoadPcx take (arg1,name); LoadPid takes
// (arg1,name,arg3)). Returns 1 unless the extension matched but its loader
// failed (then 0); an unrecognised/absent extension also returns 1.
// ---------------------------------------------------------------------------
int RezMgr::MakeImageKey(void *arg1, char *name, void *arg3)
{
    char *ext = RezStrrchr(name, '.');
    if (ext && RezStricmp(ext, s_extBmp) == 0) {
        if (!LoadBmp(arg1, name))
            return 0;
    } else if (ext && RezStricmp(ext, s_extPcx) == 0) {
        if (!LoadPcx(arg1, name))
            return 0;
    } else if (ext && RezStricmp(ext, s_extPid) == 0) {
        if (!LoadPid(arg1, name, arg3))
            return 0;
    }
    return 1;
}

// The runtime low-detail / front-end-class selector (binary @0x6455d4).
int g_rezLowDetail;

// The archive base names / path templates - file-scope literals (reloc-masked).
static const char s_rezName[]    = "Gruntz.REZ";    // 0x60c674
static const char s_join[]       = "%s\\%s";        // 0x60c66c
static const char s_dataPath[]   = "%c:\\DATA\\%s";  // 0x611054
static const char s_fecName[]    = "Gruntz.FEC";    // 0x611044
static const char s_fecLoName[]  = "GruntzLo.FEC";  // 0x611034
static const char s_moviezPath[] = "%c:\\MOVIEZ\\%s"; // 0x611024

// ---------------------------------------------------------------------------
// RezMgr::MakeRezPath()  @ 0x091670 (684 B, thiscall ret).
// Assembles the candidate archive paths (the main Gruntz.REZ and the front-end
// Gruntz.FEC / GruntzLo.FEC) and probes them with FileExists, recording in
// m_inGameDir/m_haveRez/m_haveMoviez which were found. Reports an error and
// returns 0 if nothing was found, else 1.
//
// PLATEAU 91.87% (documented): a >512 B C++ EH-frame function with four
// ref-counted MFC CString locals (one COW copy-ctor selecting the lo/hi FEC
// variant), the engine sprintf-style CString::Format wrapper, a runtime
// low-detail global branch, and FileExists probes. All call/string/IAT/EH
// operands are reloc-masked. The logic, control flow, all RezMgr member offsets
// (+0xec/+0xf0 path CStrings, +0xf4/+0xf8/+0xfc flags) and the string-template
// order are reconstructed faithfully and verified against the disasm. The sole
// residue is an EH-state-tracking write: the target advances the C++ EH state to
// 0 (`mov [esp+ehstate],ebp`) inline right before the first CString::Format,
// just after `m_haveRez=0` - my build omits exactly that one inline state-write,
// which shifts the instruction alignment by one and cascades objdiff's
// edit-distance. This is the MSVC5 EH-state scheduling over four overlapping
// CString live ranges (entropy-class; no source lever flips a single funclet
// state-write). MakeImageKey (the other target) is BYTE-EXACT and is the green
// deliverable; per the prompt's "don't sacrifice a green fn", this is left as a
// documented plateau with the full reconstruction in place.
// ---------------------------------------------------------------------------
int RezMgr::MakeRezPath()
{
    char cwd[0x100];
    if (!GetCurrentDirectoryA(0xff, cwd))
        return 0;

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    int found = 1;

    // --- main archive: cwd\Gruntz.REZ, fall back to <drive>:\DATA\Gruntz.REZ ---
    {
        AfxString rez(s_rezName);
        m_haveRez = 0;
        RezFormat(&m_pathA, s_join, cwd, (const char *)rez);
        if (!RezFileExists(m_pathA)) {
            if (drive) {
                RezFormat(&m_pathA, s_dataPath, drive, (const char *)rez);
                if (RezFileExists(m_pathA))
                    m_haveRez = 1;
                else
                    found = 0;
            } else {
                found = 0;
            }
        }
    }

    // --- front-end archive: cwd\<FEC>, then <drive>:\MOVIEZ\<FEC> ---
    AfxString fecHi(s_fecName);
    AfxString fecLo(s_fecLoName);
    AfxString fec(g_rezLowDetail ? fecLo : fecHi);

    m_haveMoviez = 0;
    int movFound = 0;
    RezFormat(&m_pathB, s_join, cwd, (const char *)fec);
    if (!m_inGameDir && !RezFileExists(m_pathB) && !g_rezLowDetail) {
        RezFormat(&m_pathB, s_join, cwd, (const char *)fecHi);
        if (RezFileExists(m_pathB))
            movFound = 1;
    }
    if (!movFound && drive) {
        RezFormat(&m_pathB, s_moviezPath, drive, (const char *)fec);
        if (RezFileExists(m_pathB))
            m_haveMoviez = 1;
    }

    if (!found) {
        ReportError(0x800b, 0x43e);
        return 0;
    }
    return 1;
}
