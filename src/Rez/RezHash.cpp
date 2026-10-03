#include <StdAfx.h>

#include <Ints.h>

#include <Rez/RezHash.h>

#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>

#include <string.h>

u32 CRezItmHashByName::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezItm->GetName());
}

u32 CRezItmHashTableByName::HashFunc(const char* text) {
    if (text == NULL) {
        return 0;
    }
    u32 count;
    for (count = 0; *text != '\0'; text++) {
        count++;
    }
    return count % GetNumBins();
}

CRezItm* CRezItmHashTableByName::Find(const char* name, i32 ignoreCase) {
    if (name == NULL) {
        return NULL;
    }
    CRezItmHashByName* item = GetFirstInBin(HashFunc(name));
    if (ignoreCase) {
        while (item != NULL) {
            if (stricmp(item->GetRezItm()->GetName(), name) == 0) {
                return item->GetRezItm();
            }
            item = item->NextInBin();
        }
    } else {
        while (item != NULL) {
            if (strcmp(item->GetRezItm()->GetName(), name) == 0) {
                return item->GetRezItm();
            }
            item = item->NextInBin();
        }
    }
    return NULL;
}

u32 CRezTypeHash::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezTyp->GetType());
}

u32 CRezTypeHashTable::HashFunc(u32 type) {
    return type % GetNumBins();
}

CRezTyp* CRezTypeHashTable::Find(u32 type) {
    CRezTypeHash* item = GetFirstInBin(HashFunc(type));
    while (item != NULL) {
        if (static_cast<u32>(item->GetRezTyp()->GetType()) == type) {
            return item->GetRezTyp();
        }
        item = item->NextInBin();
    }
    return NULL;
}

u32 CRezDirHash::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezDir->GetDirName());
}

u32 CRezDirHashTable::HashFunc(const char* text) {
    if (text == NULL) {
        return 0;
    }
    u32 count;
    for (count = 0; *text != '\0'; text++) {
        count++;
    }
    return count % GetNumBins();
}

CRezDir* CRezDirHashTable::Find(const char* name, i32 ignoreCase) {
    if (name == NULL) {
        return NULL;
    }
    CRezDirHash* item = GetFirstInBin(HashFunc(name));
    if (ignoreCase) {
        while (item != NULL) {
            if (stricmp(item->GetRezDir()->GetDirName(), name) == 0) {
                return item->GetRezDir();
            }
            item = item->NextInBin();
        }
    } else {
        while (item != NULL) {
            if (strcmp(item->GetRezDir()->GetDirName(), name) == 0) {
                return item->GetRezDir();
            }
            item = item->NextInBin();
        }
    }
    return NULL;
}

void CBaseRezFileList::VirtualFoo() {}
