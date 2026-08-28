#include <rva.h>

#include <Rez/RezHash.h>

#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>

#include <string.h>

RVA(0x0013c230, 0xf)
u32 CRezItmHashByName::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezItm->GetName());
}

RVA(0x0013c240, 0x29)
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

RVA(0x0013c270, 0xca)
CRezArchiveEntry* CRezItmHashTableByName::Find(const char* name, i32 ignoreCase) {
    if (name == NULL) {
        return NULL;
    }
    CRezItmHashByName* item = GetFirstInBin(HashFunc(name));
    if (ignoreCase) {
        while (item != NULL) {
            if (_strcmpi(item->GetRezItm()->GetName(), name) == 0) {
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

RVA(0x0013c340, 0xf)
u32 CRezTypeHash::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezTyp->GetType());
}

RVA(0x0013c350, 0xd)
u32 CRezTypeHashTable::HashFunc(u32 type) {
    return type % GetNumBins();
}

RVA(0x0013c360, 0x47)
CRezArchiveType* CRezTypeHashTable::Find(u32 type) {
    CRezTypeHash* item = GetFirstInBin(HashFunc(type));
    while (item != NULL) {
        if (static_cast<u32>(item->GetRezTyp()->GetType()) == type) {
            return item->GetRezTyp();
        }
        item = item->NextInBin();
    }
    return NULL;
}

RVA(0x0013c3b0, 0xf)
u32 CRezDirHash::HashFunc() {
    return GetParentHash()->HashFunc(m_pRezDir->GetDirName());
}

RVA(0x0013c3c0, 0x29)
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

RVA(0x0013c3f0, 0xca)
CRezArchiveDir* CRezDirHashTable::Find(const char* name, i32 ignoreCase) {
    if (name == NULL) {
        return NULL;
    }
    CRezDirHash* item = GetFirstInBin(HashFunc(name));
    if (ignoreCase) {
        while (item != NULL) {
            if (_strcmpi(item->GetRezDir()->GetDirName(), name) == 0) {
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

RVA(0x0013c4c0, 0x1)
void CRezStorageList::UnusedListHook() {}
