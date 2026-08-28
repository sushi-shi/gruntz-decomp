#include <rva.h>

#include <Bute/Hash.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>

RVA(0x0013c230, 0xf)
u32 CRezArchiveEntryHashNode::Hash() {
    return static_cast<CRezEntryNameHash*>(m_hash)->HashStr(m_archiveEntry->m_name);
}

RVA(0x0013c240, 0x29)
u32 CRezEntryNameHash::HashStr(const char* text) {
    if (!text) {
        return 0;
    }
    u32 length = 0;
    while (*text) {
        ++length;
        ++text;
    }
    return length % m_bucketCount;
}

RVA(0x0013c270, 0xca)
CRezArchiveEntry* CRezEntryNameHash::FindByName(const char* name, i32 caseInsensitive) {
    if (!name) {
        return NULL;
    }
    CRezArchiveEntryHashNode* node = Lookup(HashStr(name));
    if (caseInsensitive) {
        while (node) {
            const char* entryName = node->GetArchiveEntry()->m_name;
            if (_strcmpi(entryName, name) == 0) {
                return node->GetArchiveEntry();
            }
            node = node->NextInBucket();
        }
        return NULL;
    }
    while (node) {
        const char* entryName = node->GetArchiveEntry()->m_name;
        if (strcmp(entryName, name) == 0) {
            return node->GetArchiveEntry();
        }
        node = node->NextInBucket();
    }
    return NULL;
}

RVA(0x0013c340, 0xf)
u32 CRezArchiveTypeHashNode::Hash() {
    return static_cast<CRezTypeTagHash*>(m_hash)->HashTypeTag(m_archiveType->m_typeTag);
}

RVA(0x0013c350, 0xd)
u32 CRezTypeTagHash::HashTypeTag(u32 typeTag) {
    return typeTag % m_bucketCount;
}

RVA(0x0013c360, 0x47)
CRezArchiveType* CRezTypeTagHash::FindTypeByTag(u32 typeTag) {
    CRezArchiveTypeHashNode* node = Lookup(HashTypeTag(typeTag));
    while (node) {
        if (static_cast<u32>(node->GetArchiveType()->m_typeTag) == typeTag) {
            return node->GetArchiveType();
        }
        node = node->NextInBucket();
    }
    return NULL;
}

RVA(0x0013c3b0, 0xf)
u32 CRezArchiveDirHashNode::Hash() {
    return static_cast<CRezDirectoryNameHash*>(m_hash)->HashStr(m_archiveDirectory->m_name);
}

RVA(0x0013c3c0, 0x29)
u32 CRezDirectoryNameHash::HashStr(const char* text) {
    if (!text) {
        return 0;
    }
    u32 length = 0;
    while (*text) {
        ++length;
        ++text;
    }
    return length % m_bucketCount;
}

RVA(0x0013c3f0, 0xca)
CRezArchiveDir* CRezDirectoryNameHash::FindByName(const char* name, i32 caseInsensitive) {
    if (!name) {
        return NULL;
    }
    CRezArchiveDirHashNode* node = Lookup(HashStr(name));
    if (caseInsensitive) {
        while (node) {
            const char* directoryName = node->GetArchiveDirectory()->m_name;
            if (_strcmpi(directoryName, name) == 0) {
                return node->GetArchiveDirectory();
            }
            node = node->NextInBucket();
        }
        return NULL;
    }
    while (node) {
        const char* directoryName = node->GetArchiveDirectory()->m_name;
        if (strcmp(directoryName, name) == 0) {
            return node->GetArchiveDirectory();
        }
        node = node->NextInBucket();
    }
    return NULL;
}

RVA(0x0013c4c0, 0x1)
void CRezStorageList::UnusedListHook() {}
