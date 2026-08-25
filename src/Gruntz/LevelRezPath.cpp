#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileStream.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
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

i32 CGruntzMgr::ResolveLevelChecksum(
    i32 useDirectLevelReference,
    i32 isBattlez,
    i32 isCustom,
    i32 levelId,
    CString levelName
) {
    if (isCustom != 0) {
        WwdHeader buf;
        CFile file;
        CString path;
        if (useDirectLevelReference == 0 && isBattlez == 0) {
            path = "custom\\" + levelName;
        } else {
            path = levelName;
        }
        if (file.Open(path, 0, NULL)) {
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

    if (useDirectLevelReference == 0) {
        if (isBattlez != 0) {
            WwdHeader buf;
            CRezArchiveDir* node = m_resourceArchive->FindDirectoryByPath("GAME_BATTLEZ");
            if (node == NULL) {
                return 0;
            }
            CRezArchiveEntry* sub = node->FindEntry(levelName, REZ_TAG_WWD);
            if (sub == NULL) {
                return 0;
            }
            char* parsed = sub->LoadData();
            if (parsed == NULL) {
                return 0;
            }
            memcpy(&buf, parsed, 0x5f4);
            sub->ReleaseData();
            return buf.checksum;
        } else {
            WwdHeader buf;
            CRezArchiveDir* node = m_resourceArchive->FindDirectoryByPath("GAME_MULTI");
            if (node == NULL) {
                return 0;
            }
            CRezArchiveEntry* sub = node->FindEntry(levelName, REZ_TAG_WWD);
            if (sub == NULL) {
                return 0;
            }
            char* parsed = sub->LoadData();
            if (parsed == NULL) {
                return 0;
            }
            memcpy(&buf, parsed, 0x5f4);
            sub->ReleaseData();
            return buf.checksum;
        }
    } else {
        // `scratch` shares the CFile slot ([esp+0x18]) and the union region is 0x20
        // wide (buf lands at [esp+0x38]) - so it is a 32-byte buffer local to this arm.
        WwdHeader buf;
        char scratch[32];
        sprintf(scratch, "AREA%i_WORLDZ", ((levelId - 1) % 0x24) / 4 + 1);
        CRezArchiveDir* node = m_resourceArchive->FindDirectoryByPath(scratch);
        if (node == NULL) {
            return 0;
        }
        if (levelId > 0x24) {
            sprintf(scratch, "TRAINING%i", levelId % 0x24);
        } else {
            sprintf(scratch, "LEVEL%i", levelId);
        }
        CRezArchiveEntry* sub = node->FindEntry(scratch, REZ_TAG_WWD);
        if (sub == NULL) {
            return 0;
        }
        char* parsed = sub->LoadData();
        if (parsed == NULL) {
            return 0;
        }
        memcpy(&buf, parsed, 0x5f4);
        sub->ReleaseData();
        return buf.checksum;
    }
}

DATA(0x00245510)
FILE* g_logFile;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000942e0, 0x18)
void OpenDebugLog() {
    g_logFile = fopen("c:\\foo.log", "wb");
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00094310, 0x1d)
void CloseDebugLog() {
    if (g_logFile != NULL) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}
