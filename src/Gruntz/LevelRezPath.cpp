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
RVA(0x00093d40, 0x473)

i32 CGruntzMgr::BuildLevelRezPath(i32 isEmpty, i32 hi, i32 lo, i32 id, CString name) {
    char scratch[16];
    WwdHeader buf;
    if (lo != 0) {
        CFile file;
        CString path;
        if (isEmpty == 0 && hi == 0) {
            path = "custom\\" + name;
        } else {
            path = name;
        }
        if (file.Open(path, 0, 0)) {
            if (file.GetLength() >= 0x5f4) {
                file.Read(&buf, sizeof(buf));
                file.Close();
                return buf.checksum;
            }
            file.Close();
        }
        return 0;
    }

    if (isEmpty != 0) {
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
    if (hi == 0) {
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
}
