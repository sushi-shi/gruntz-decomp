#include <rva.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Enums.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ParseSource.h>
#include <Io/FileStream.h>
#include <Rez/RezTypeTag.h>
#include <Wwd/WwdFile.h>

#include <stdio.h>
#include <string.h>

// @early-stop
// One residue: cl materialises the EH state 1 as an immediate where retail claims a
// 4th callee-saved register for it (`push ebx` / `mov bl,1` / `mov [esp+N],bl` x3),
// which is the uniform +4 on every [esp+N].  See const-materialize-into-reg-vs-immediate.
// Each arm owns its OWN `WwdHeader buf` - that is what stops cl cross-jumping the
// BATTLEZ and MULTI arms into one shared tail (docs/patterns/identical-arms-need-
// distinct-locals.md).
RVA(0x00093d40, 0x473)

i32 CGruntzMgr::BuildLevelRezPath(i32 isEmpty, i32 hi, i32 lo, i32 id, CString name) {
    if (lo != 0) {
        WwdHeader buf;
        CFile file;
        CString path;
        if (isEmpty == 0 && hi == 0) {
            path = "custom\\" + name;
        } else {
            path = name;
        }
        if (file.Open(path, 0, 0)) {
            if (file.GetLength() < 0x5f4) {
                file.Close();
            } else {
                file.Read(&buf, sizeof(buf));
                file.Close();
                return buf.checksum;
            }
        }
        return 0;
    }

    if (isEmpty == 0) {
        if (hi != 0) {
            WwdHeader buf;
            CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME_BATTLEZ"));
            if (node == NULL) {
                return 0;
            }
            CParseSource* sub = node->Insert(name, REZ_TAG_WWD);
            if (sub == NULL) {
                return 0;
            }
            void* parsed = sub->BeginParse();
            if (parsed == NULL) {
                return 0;
            }
            memcpy(&buf, parsed, 0x5f4);
            sub->EndParse();
            return buf.checksum;
        } else {
            WwdHeader buf;
            CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME_MULTI"));
            if (node == NULL) {
                return 0;
            }
            CParseSource* sub = node->Insert(name, REZ_TAG_WWD);
            if (sub == NULL) {
                return 0;
            }
            void* parsed = sub->BeginParse();
            if (parsed == NULL) {
                return 0;
            }
            memcpy(&buf, parsed, 0x5f4);
            sub->EndParse();
            return buf.checksum;
        }
    } else {
        // `scratch` shares the CFile slot ([esp+0x18]) and the union region is 0x20
        // wide (buf lands at [esp+0x38]) - so it is a 32-byte buffer local to this arm.
        WwdHeader buf;
        char scratch[32];
        sprintf(scratch, "AREA%i_WORLDZ", ((id - 1) % 0x24) / 4 + 1);
        CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath(scratch));
        if (node == NULL) {
            return 0;
        }
        if (id > 0x24) {
            sprintf(scratch, "TRAINING%i", id % 0x24);
        } else {
            sprintf(scratch, "LEVEL%i", id);
        }
        CParseSource* sub = node->Insert(scratch, REZ_TAG_WWD);
        if (sub == NULL) {
            return 0;
        }
        void* parsed = sub->BeginParse();
        if (parsed == NULL) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->EndParse();
        return buf.checksum;
    }
}

DATA(0x00245510)
FILE* g_logFile;

RVA(0x000942e0, 0x18)
void OpenDebugLog() {
    g_logFile = fopen("c:\\foo.log", "wb");
}

RVA(0x00094310, 0x1d)
void CloseDebugLog() {
    if (g_logFile != NULL) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}
