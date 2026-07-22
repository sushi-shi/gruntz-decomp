#include <Mfc.h>           // MFC CString (ctor/dtor/op=/op+, all reloc-masked)
#include <Io/FileStream.h> // CFileIO (0x1befd7 ctor / 0x1bf121 dtor / Open/Read/GetLength/Close)
#include <string.h>        // inline memcpy (rep movsd) at /O2
#include <stdio.h>         // sprintf (0x11f890)

#include <rva.h>

#include <Bute/SymParser.h>     // the shared CSymParser (ResolvePath 0x13c030)
#include <Gruntz/GruntzMgr.h>   // the REAL owner (was the LevelRezLoader view)
#include <Gruntz/LevelRezPath.h> // LevelRezData (the 0x5f4 rez descriptor)
#include <Gruntz/ParseSource.h> // CParseSource::BeginParse/EndParse (0x139960/0x1399d0)

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
i32 CGruntzMgr::BuildLevelRezPath(i32 a1, i32 a2, i32 a3, i32 a4, CString name) {
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
        sprintf(scratch, "AREA%i_WORLDZ", ((a4 - 1) % 0x24) / 4 + 1);
        CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath(scratch));
        if (node == 0) {
            return 0;
        }
        if (a4 > 0x24) {
            sprintf(scratch, "TRAINING%i", a4 % 0x24);
        } else {
            sprintf(scratch, "LEVEL%i", a4);
        }
        CParseSource* sub = node->Insert(scratch, reinterpret_cast<void*>(0x575744));
        if (sub == 0) {
            return 0;
        }
        void* parsed = reinterpret_cast<void*>(sub->BeginParse());
        if (parsed == 0) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->EndParse();
        return buf.m_2ec;
    }
    if (a2 == 0) {
        CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME_MULTI"));
        if (node == 0) {
            return 0;
        }
        CParseSource* sub =
            node->Insert(name, reinterpret_cast<void*>(0x575744));
        if (sub == 0) {
            return 0;
        }
        void* parsed = reinterpret_cast<void*>(sub->BeginParse());
        if (parsed == 0) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->EndParse();
        return buf.m_2ec;
    }
    CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME_BATTLEZ"));
    if (node == 0) {
        return 0;
    }
    CParseSource* sub =
        node->Insert(name, reinterpret_cast<void*>(0x575744));
    if (sub == 0) {
        return 0;
    }
    void* parsed = reinterpret_cast<void*>(sub->BeginParse());
    if (parsed == 0) {
        return 0;
    }
    memcpy(&buf, parsed, 0x5f4);
    sub->EndParse();
    return buf.m_2ec;
}

