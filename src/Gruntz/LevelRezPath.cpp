// LevelRezPath.cpp - BuildLevelRezPath (0x93d40), the level/battlez rez-data
// loader, a __thiscall(this; a1,a2,a3,a4, CString name) (ret 0x14) that returns
// the parsed descriptor's +0x2ec field (0 on any failure). The by-value CString
// arg + the CFileIO / CString locals give it the /GX exception frame, so it lives
// in its own `eh` unit (graduated out of the engine_label_stubs aggregate).
//
// Structure (recovered dump_target + decomp + string_xref):
//   - a3 != 0: the CUSTOM-file path. Build the file name ("custom\" + name when
//     a1==a2==0, else name), CFileIO::Open it; when it opens AND is >= 0x5f4 bytes,
//     Read 0x5f4 bytes into a stack descriptor and return its +0x2ec field; else 0.
//   - a3 == 0: the namespace path. Resolve one of three rez trees off this->m_34
//     (a1!=0 -> "AREA%i_WORLDZ" for the world index; a2==0 -> "GAME_MULTI"; else
//     "GAME_BATTLEZ"), Insert the level key ("LEVEL%i"/"TRAINING%i" for the AREA
//     path, else `name`), BeginParse -> memcpy 0x5f4 bytes -> EndParse, and return
//     the descriptor's +0x2ec field. Any resolve/insert/parse failure returns 0.
//
// CARCASS doctrine: the owning class + the ButeMgr symbol tree are engine classes
// reached by raw this+offset; every callee is a reloc-masked external; the format
// / namespace strings are $SG literals reloc-masked against the matched string
// symbols. The Insert() flags arg (0x575744) is a NON-relocated literal constant
// in retail (verified against .reloc), so it is written as a bare immediate.
#include <Mfc.h>           // MFC CString (ctor/dtor/op=/op+, all reloc-masked)
#include <Io/FileStream.h> // CFileIO (0x1befd7 ctor / 0x1bf121 dtor / Open/Read/GetLength/Close)
#include <string.h>        // inline memcpy (rep movsd) at /O2

#include <rva.h>

#include <Bute/SymParser.h> // the shared CSymParser (ResolvePath 0x13c030)

// The ButeMgr symbol tree the rez data is resolved/parsed through.
struct CSymNode {
    CSymNode* Insert(const char* key, i32 flags); // 0x13a000 (2nd arg is a non-reloc literal)
    void* BeginParse();                           // 0x139960  returns the 0x5f4-byte block
    void EndParse();                              // 0x1399d0
};

// The 0x5f4-byte rez descriptor read/parsed into a stack buffer; only its +0x2ec
// field is returned.
struct LevelRezData {
    char m_pad00[0x2ec];
    i32 m_2ec; // +0x2ec  returned field
    char m_pad2f0[0x5f4 - 0x2f0];
};

// FormatBuf (0x11f890): the engine sprintf-into-char-buffer, __cdecl variadic.
extern "C" i32 FormatBuf(char* buf, const char* fmt, ...); // 0x11f890

class LevelRezLoader {
public:
    i32 BuildLevelRezPath(i32 a1, i32 a2, i32 a3, i32 a4, CString name);

    char m_pad00[0x34];
    CSymParser* m_34; // +0x34  the rez-tree parser
};

// @source: decomp-xref
// @early-stop
// zero-register-pinning wall (~45%): logic + control flow + every call/offset are
// byte-identical to retail EXCEPT retail pins the /GX EH-state constant 1 in `bl`
// (`mov bl,1; mov [state],bl`, 4 state=1 stores vs the recompile's 3) which forces a
// 4th callee-saved `push ebx`; that extra push shifts EVERY [esp+disp] operand by 4,
// cascading a fuzzy-score hit through the whole body. Verified instruction-by-
// instruction (llvm-objdump -dr, base vs delinked target): identical opcodes, disp32
// differs by 4 throughout; the only other residual is the demangled-vs-mangled EH/
// CString reloc names. Documented, not source-steerable: docs/patterns/zero-register-pinning.md.
RVA(0x00093d40, 0x473)
i32 LevelRezLoader::BuildLevelRezPath(i32 a1, i32 a2, i32 a3, i32 a4, CString name) {
    char scratch[16];
    LevelRezData buf;
    if (a3 != 0) {
        CFileIO file;
        CString path;
        if (a1 == 0 && a2 == 0) {
            path = "custom\\" + name;
        } else {
            path = name;
        }
        if (file.Open(path, 0, 0)) {
            if (file.GetLength() >= 0x5f4) {
                file.Read(&buf, 0x5f4);
                file.Close();
                return buf.m_2ec;
            }
            file.Close();
        }
        return 0;
    }

    // Namespace path. Retail writes the Insert/BeginParse/memcpy/EndParse tail out
    // inline per sub-path (no factoring), so it is duplicated here to match.
    if (a1 != 0) {
        FormatBuf(scratch, "AREA%i_WORLDZ", ((a4 - 1) % 0x24) / 4 + 1);
        CSymNode* node = (CSymNode*)m_34->ResolvePath(scratch);
        if (node == 0) {
            return 0;
        }
        if (a4 > 0x24) {
            FormatBuf(scratch, "TRAINING%i", a4 % 0x24);
        } else {
            FormatBuf(scratch, "LEVEL%i", a4);
        }
        CSymNode* sub = node->Insert(scratch, 0x575744);
        if (sub == 0) {
            return 0;
        }
        void* parsed = sub->BeginParse();
        if (parsed == 0) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->EndParse();
        return buf.m_2ec;
    }
    if (a2 == 0) {
        CSymNode* node = (CSymNode*)m_34->ResolvePath("GAME_MULTI");
        if (node == 0) {
            return 0;
        }
        CSymNode* sub = node->Insert(name, 0x575744);
        if (sub == 0) {
            return 0;
        }
        void* parsed = sub->BeginParse();
        if (parsed == 0) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->EndParse();
        return buf.m_2ec;
    }
    CSymNode* node = (CSymNode*)m_34->ResolvePath("GAME_BATTLEZ");
    if (node == 0) {
        return 0;
    }
    CSymNode* sub = node->Insert(name, 0x575744);
    if (sub == 0) {
        return 0;
    }
    void* parsed = sub->BeginParse();
    if (parsed == 0) {
        return 0;
    }
    memcpy(&buf, parsed, 0x5f4);
    sub->EndParse();
    return buf.m_2ec;
}

SIZE_UNKNOWN(CSymNode);
SIZE_UNKNOWN(LevelRezData);
SIZE_UNKNOWN(LevelRezLoader);
