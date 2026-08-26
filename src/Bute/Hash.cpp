#include <rva.h>

#include <Bute/Hash.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>

#include <string.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @identity-TODO: the owner, field, one-argument ABI, and copy bound are proven; the name is inferred.
RVA(0x0013c4d0, 0x24)
void CRezArchive::SetBannerLine2(const char* text) {
    strncpy(m_bannerLine2, text, REZ_ARCHIVE_BANNER_TEXT_LENGTH);
    m_bannerLine2[REZ_ARCHIVE_BANNER_TEXT_LENGTH] = 0;
}

RVA(0x0013c500, 0x14)
u32 CRezArchiveEntryHashNode::Hash() {
    if (m_archiveEntry == NULL) {
        return 0;
    }
    return static_cast<CRezEntryNameHash*>(m_hash)->HashStr(m_archiveEntry->m_name);
}

RVA(0x0013c520, 0x29)
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

RVA(0x0013c550, 0xca)
CRezArchiveEntry* CRezEntryNameHash::FindByName(const char* name, i32 caseInsensitive) {
    if (!name) {
        return NULL;
    }
    CHashElement* node = Lookup(HashStr(name));
    if (caseInsensitive) {
        while (node) {
            const char* entryName = node->m_archiveEntry->m_name;
            if (_strcmpi(entryName, name) == 0) {
                return node->m_archiveEntry;
            }
            node = FromLink(node->m_next);
        }
        return NULL;
    }
    while (node) {
        const char* entryName = node->m_archiveEntry->m_name;
        if (strcmp(entryName, name) == 0) {
            return node->m_archiveEntry;
        }
        node = FromLink(node->m_next);
    }
    return NULL;
}

RVA(0x0013c620, 0xf)
u32 CRezArchiveTypeHashNode::Hash() {
    return static_cast<CRezTypeTagHash*>(m_hash)->HashTypeTag(m_archiveType->m_typeTag);
}

RVA(0x0013c630, 0xd)
u32 CRezTypeTagHash::HashTypeTag(u32 typeTag) {
    return typeTag % m_bucketCount;
}

RVA(0x0013c640, 0x47)
CRezArchiveType* CRezTypeTagHash::FindTypeByTag(u32 typeTag) {
    CHashElement* node = Lookup(HashTypeTag(typeTag));
    while (node) {
        if (static_cast<u32>(node->m_archiveType->m_typeTag) == typeTag) {
            return node->m_archiveType;
        }
        node = FromLink(node->m_next);
    }
    return NULL;
}

RVA(0x0013c690, 0xf)
u32 CRezArchiveDirHashNode::Hash() {
    return static_cast<CRezDirectoryNameHash*>(m_hash)->HashStr(m_archiveDirectory->m_name);
}

RVA(0x0013c6a0, 0x29)
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

RVA(0x0013c6d0, 0xca)
CRezArchiveDir* CRezDirectoryNameHash::FindByName(const char* name, i32 caseInsensitive) {
    if (!name) {
        return NULL;
    }
    CHashElement* node = Lookup(HashStr(name));
    if (caseInsensitive) {
        while (node) {
            const char* directoryName = node->m_archiveDirectory->m_name;
            if (_strcmpi(directoryName, name) == 0) {
                return node->m_archiveDirectory;
            }
            node = FromLink(node->m_next);
        }
        return NULL;
    }
    while (node) {
        const char* directoryName = node->m_archiveDirectory->m_name;
        if (strcmp(directoryName, name) == 0) {
            return node->m_archiveDirectory;
        }
        node = FromLink(node->m_next);
    }
    return NULL;
}

RVA(0x0013c7a0, 0x1)
void CRezStorageList::UnusedListHook() {}
