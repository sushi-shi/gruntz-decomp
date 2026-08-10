#include <rva.h>

#include <Bute/SymTab.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <Bute/SymParser.h>
#include <Dsndmgr/SoundBankLoad.h>
#include <Enums.h>
#include <Gruntz/CustomWorldInfoDlg.h>
#include <Gruntz/ParseSource.h>
#include <Pix16.h>
#include <PlacementNew.h>
#include <Rez/DebugPrintf.h>
#include <Rez/RezFile.h>
#include <Rez/RezMgr.h>
#include <Rez/RezTypeTag.h>

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

DATA(0x0020cff0)
char g_sepSlash[] = "\\";

static __inline i32 IsTokenChar(const char* delims, char ch) {
    if (delims) {
        return strchr(delims, ch) == NULL;
    }
    if (ch >= ' ' && ch <= '.') {
        return 1;
    }
    if (ch >= '0' && ch <= '9') {
        return 1;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    }
    if (ch >= 'a' && ch <= 'z') {
        return 1;
    }
    return 0;
}

static inline CSlotNode* HeadSlotNode(DSoundList& list) {
    return static_cast<CSlotNode*>(list.m_head);
}

// Retail's CSymParser::ParseRecords frame (0x1674 via __chkstk) lays out six
// path buffers of exactly 0x308 bytes; the three _splitpath component buffers
// are _MAX_PATH.
static const i32 REZ_SCAN_PATH_MAX = 0x308;

// Byte-forced view of packed serialized storage.
static inline i32 PeekI32(const char* p) {
    return *reinterpret_cast<const i32*>(p);
}

RVA(0x001396f0, 0x1a)
CParseSource::CParseSource() {

    m_reader = NULL;
    m_owner = NULL;
    m_name = NULL;
    m_node1c.m_parseSource = this;
}

// @early-stop
RVA(0x00139710, 0x8d)
void CParseSource::Build(
    CSymTab* owner,
    const char* name,
    void* f4,
    CSymRec* rec,
    void* str2,
    i32 f3,
    i32 f1,
    i32 f2,
    i32 f6,
    void* arr,
    CRezItmBase* stream
) {
    m_reader = stream;
    m_owner = owner;
    if (name == NULL) {
        m_name = const_cast<char*>(name);
    } else {
        m_name = new char[strlen(name) + 1];
        if (m_name) {
            strcpy(m_name, name);
        }
    }
    m_entry = rec;
    m_length = f3;
    m_base = f1;
    m_typeTag = f2;
    m_buffer = NULL;
    m_cursor = 0;
    m_node1c.m_parseSource = this;
}

RVA(0x001397a0, 0x57)
void CParseSource::Teardown() {
    if (m_name) {
        delete[] m_name;
    }
    if (m_owner != NULL) {
        if (m_owner->m_mappedBuf == NULL) {
            if (m_buffer) {
                delete[] m_buffer;
            }
        }
    } else {
        if (m_buffer) {
            delete[] m_buffer;
        }
    }
    m_name = NULL;
    m_entry = NULL;
    m_typeTag = 0;
    m_length = 0;
    m_buffer = NULL;
    m_owner = NULL;
    m_base = 0;
    m_cursor = 0;
    m_node1c.m_parseSource = NULL;
}

RVA(0x00139800, 0x6)
GZ_ENUM_RETURN(RezTypeTag, u32) CParseSource::GetEntryTag() {
    // CSymRec::m_key is the generic symbol key; for a REZ entry it holds the
    // entry tag, which is what this accessor exists to expose.
    return static_cast<RezTypeTag>(m_entry->m_key);
}

RVA(0x00139810, 0x140)
char* CParseSource::CurrentScopePath(char* dst, i32 size) {
    if (m_owner->m_parent == NULL) {
        strcpy(dst, g_sepSlash);
    } else {
        char* scratch = new char[size];
        strcpy(dst, "");
        CSymTab* node = m_owner;
        while (node != NULL) {
            strcpy(scratch, dst);
            if (node->m_parent != NULL) {
                strcpy(dst, g_sepSlash);
            } else {
                dst[0] = 0;
            }
            strcat(dst, node->m_name);
            strcat(dst, scratch);
            node = node->m_parent;
        }
        delete[] scratch;
    }
    return dst;
}

RVA(0x00139950, 0x6)
char* CParseSource::CurrentScopeName() {
    return m_owner->m_name;
}

RVA(0x00139960, 0x6b)
char* CParseSource::BeginParse() {
    if (m_owner->m_mappedBuf != NULL) {
        return m_owner->m_mappedBuf + (m_base - m_owner->m_baseOffset);
    }
    if (m_buffer != NULL) {
        return m_buffer;
    }
    if (m_length == 0) {
        return 0;
    }
    m_buffer = new char[m_length];
    if (m_buffer == NULL) {
        return 0;
    }
    if (m_reader->Read(m_base, 0, m_length, m_buffer) != static_cast<i32>(m_length)) {
        delete[] m_buffer;
        m_buffer = NULL;
    }
    return m_buffer;
}

RVA(0x001399d0, 0x21)
i32 CParseSource::EndParse() {
    if (m_buffer != NULL) {
        delete[] m_buffer;
        m_buffer = NULL;
    }
    return 1;
}

RVA(0x00139a00, 0x1b)
i32 CParseSource::IsResident() {
    if (m_owner->m_mappedBuf != NULL) {
        return 1;
    }
    return m_buffer != NULL;
}

RVA(0x00139a20, 0x13)
i32 CParseSource::ReadAll(void* dst) {
    return ReadAt(dst, 0, m_length);
}

RVA(0x00139a40, 0x95)
i32 CParseSource::ReadAt(void* dst, i32 pos, u32 len) {
    CSymTab* sd = m_owner;
    if (sd->m_mappedBuf != NULL) {
        memcpy(dst, sd->m_mappedBuf + (m_base - sd->m_baseOffset + pos), len);
        return 1;
    }
    if (m_buffer != NULL) {
        memcpy(dst, (m_buffer + pos), len);
        return 1;
    }
    return m_reader->Read(m_base, pos, len, dst) == static_cast<i32>(len);
}

RVA(0x00139ae0, 0xf)
i32 CParseSource::SetPos(i32 pos) {
    m_cursor = pos;
    return 1;
}

// @early-stop
RVA(0x00139af0, 0xcc)
i32 CParseSource::Read(void* dst, u32 len, i32 seekPos) {
    if (seekPos != -1) {
        SetPos(seekPos);
    }

    u32 pos = static_cast<u32>(m_cursor);
    u32 want = len;
    if (want + pos > m_length) {
        want = m_length - pos;
    }

    if (want > 0) {
        CSymTab* sd = m_owner;
        if (sd->m_mappedBuf) {

            const char* base = m_base - sd->m_baseOffset + sd->m_mappedBuf;
            base += pos;
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_buffer) {
            const char* base = (m_buffer + pos);
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_reader->Read(m_base, pos, want, dst) == static_cast<i32>(want)) {
            m_cursor += want;
            return want;
        }
    }
    return 0;
}

RVA(0x00139bc0, 0xc)
i32 CParseSource::AtEnd() {
    return static_cast<u32>(m_cursor) >= m_length;
}

RVA(0x00139bd0, 0x15)
char CParseSource::ReadChar() {
    char c;
    Read(&c, 1, -1);
    return c;
}

// @early-stop
RVA(0x00139bf0, 0x71)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c, i32 d) : m_keyTable(c), m_valTable(d) {
    m_symNode.m_symRec = this;
    m_scope = owner;
    m_key = key;
}

// cl's unwind helper for the member-init list above: an out-of-line copy of the
// inline ~CHash(), which is just `RemoveAll()` and so tail-jumps to the base.
RVA_COMPGEN(0x00139c70, 0x5, ??1CHash@@QAE@XZ)

RVA(0x00139c80, 0x6c)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c) : m_keyTable(), m_valTable(c) {
    m_key = key;
    m_symNode.m_symRec = this;
    m_scope = owner;
}

// @early-stop
RVA(0x00139cf0, 0xd7)
CSymRec::~CSymRec() {
    if (m_scope->m_owner->m_useKeyIndex != 0) {
        CHashElement* n = m_keyTable.First();
        while (n) {
            CHashElement* cur = n;
            n = cur->Next();
            m_keyTable.Remove(cur);
        }
    }
    CHashElement* n = m_valTable.First();
    while (n) {
        CHashElement* cur = n;
        n = cur->Next();
        m_valTable.Remove(cur);
        cur->m_parseSource->Teardown();
        m_scope->m_owner->AddNode(cur->m_parseSource);
    }
    m_key = 0;
    m_symNode.m_symRec = NULL;
}

// ~CSymRec is the first function to unwind m_valTable (this+0x24), so cl emits
// that member's inline destructor here - a second, distinct `jmp RemoveAll`.
RVA_COMPGEN(0x00139dd0, 0x5, ??1CHashC@@QAE@XZ)

// @early-stop
RVA(0x00139de0, 0xd4)
CSymTab::CSymTab(
    CSymParser* owner,
    CSymTab* parent,
    const char* name,
    i32 dataOff,
    i32 dataSize,
    i32 dirTime,
    i32 subN,
    i32 symN
)
    : m_subTabs(subN), m_symbols(symN) {
    m_name = new char[strlen(name) + 1];
    if (m_name) {
        strcpy(m_name, name);
    }
    m_dirTime = dirTime;
    m_dataSize = dataSize;
    m_dataOff = dataOff;
    m_owner = owner;
    m_totalSourceLength = 0;
    m_baseOffset = 0;
    m_mappedBuf = NULL;
    m_parent = parent;
    m_node20.m_symTab = this;
}

// The CSymTab ctor unwinds m_subTabs (this+0x38) then m_symbols (this+0x40), and
// cl emits both members' inline destructors behind it, in that order.
RVA_COMPGEN(0x00139ec0, 0x5, ??1CHashB@@QAE@XZ)
RVA_COMPGEN(0x00139ed0, 0x5, ??1CHashD@@QAE@XZ)

RVA(0x00139ee0, 0x11e)
CSymTab::~CSymTab() {

    CHashElement* e;
    for (e = m_symbols.First(); e != NULL;) {
        CHashElement* cur = e;
        e = cur->Next();
        m_symbols.Remove(cur);
        CSymRec* rec = cur->m_symRec;
        delete rec;
    }
    for (e = m_subTabs.First(); e != NULL;) {
        CHashElement* cur = e;
        e = cur->Next();
        m_subTabs.Remove(cur);
        CSymTab* sub = cur->m_symTab;
        delete sub;
    }
    if (m_name) {
        delete[] m_name;
    }
    if (m_mappedBuf) {
        ::operator delete(m_mappedBuf);
    }
    m_name = NULL;
    m_dirTime = 0;
    m_dataSize = 0;
    m_dataOff = 0;
    m_totalSourceLength = 0;
    m_baseOffset = 0;
    m_mappedBuf = NULL;
    m_owner = NULL;
    m_parent = NULL;
    m_node20.m_symTab = NULL;
}

RVA(0x0013a000, 0x37)
CParseSource* CSymTab::Insert(const char* key, RezTypeTag fourcc) {
    CSymRec* rec = static_cast<CSymRec*>(m_symbols.FindInt(IDX(fourcc)));
    if (!rec) {
        return 0;
    }
    return static_cast<CParseSource*>(rec->m_valTable.Walk(key, m_owner->m_caseSensitive == 0));
}

RVA(0x0013a040, 0xa2)
void* CSymTab::Find(const char* path) {
    char dir[260];
    char fname[260];
    char ext[260];
    char drive[4];
    char tmp[8];
    _splitpath(path, drive, dir, fname, ext);
    RezTypeTag fourcc;
    if (strlen(ext) != 0) {
        strcpy(tmp, ext + 1);
        _strupr(tmp);
        fourcc = m_owner->PackTag(tmp);
    } else {
        fourcc = REZ_TAG_NONE;
    }
    return Insert(fname, fourcc);
}

RVA(0x0013a0f0, 0x99)
i32 CRezDirNode::Load(i32 childFlag) {
    if (m_buf != NULL) {
        return 1;
    }

    CSymParser* src = m_src;
    if (src->m_sorted == 0 || src->m_list.m_count > 1) {
        RezAssertFail("CRezDir::Load Failed! (File is not sorted!)\n");
        return 0;
    }

    if (m_size > 0) {
        m_buf = new u8[m_size];
        if (m_buf != NULL) {
            m_src->m_activeNode->Read(m_off, 0, m_size, m_buf);
        }
    }

    if (childFlag != 0) {
        for (CHashElement* n = m_kids.First(); n != NULL; n = n->Next()) {

            n->m_rezDirNode->Load(1);
        }
    }
    return 1;
}

RVA(0x0013a190, 0x94)
i32 CSymTab::ReleaseParseBuffers(i32 recurse) {
    if (m_mappedBuf != NULL) {
        ::operator delete(m_mappedBuf);
        m_mappedBuf = NULL;
    } else {
        void* rec = FirstSym();
        while (rec) {
            void* sub = NextSym2(rec);
            while (sub) {
                (static_cast<CParseSource*>(sub))->EndParse();
                sub = NextSym3(sub);
            }
            rec = NextSym(rec);
        }
    }
    if (recurse) {
        CHashElement* e = m_subTabs.First();
        while (e) {
            e->m_symTab->ReleaseParseBuffers(1);
            e = e->Next();
        }
    }
    return 1;
}

RVA(0x0013a230, 0x29)
void* CSymTab::FindSub(const char* name) {
    if (!name) {
        return const_cast<char*>(name);
    }
    return m_subTabs.Walk(name, m_owner->m_caseSensitive == 0);
}

RVA(0x0013a260, 0x11)
void* CSymTab::FirstSub() {
    CHashElement* n = m_subTabs.First();
    if (!n) {
        return n;
    }
    return n->m_symTab;
}

RVA(0x0013a280, 0x19)
void* CSymTab::NextSub(void* rec) {
    CHashElement* n = (static_cast<CSymTab*>(rec))->m_node20.Next();
    if (!n) {
        return n;
    }
    return n->m_symTab;
}

RVA(0x0013a2a0, 0x10)
void* CSymTab::FindSymKey(u32 key) {
    return m_symbols.FindInt(key);
}

RVA(0x0013a2b0, 0x11)
void* CSymTab::FirstSym() {
    CHashElement* n = m_symbols.First();
    if (!n) {
        return n;
    }
    return n->m_symRec;
}

RVA(0x0013a2d0, 0x19)
void* CSymTab::NextSym(void* rec) {
    CHashElement* n = (static_cast<CSymRec*>(rec))->m_symNode.Next();
    if (!n) {
        return n;
    }
    return n->m_symRec;
}

RVA(0x0013a2f0, 0x19)
void* CSymTab::NextSym2(void* rec) {
    CHashElement* n = (static_cast<CSymRec*>(rec))->m_valTable.First();
    if (!n) {
        return n;
    }
    return n->m_parseSource;
}

RVA(0x0013a310, 0x19)
void* CSymTab::NextSym3(void* rec) {
    CHashElement* n = (static_cast<CParseSource*>(rec))->m_node1c.Next();
    if (!n) {
        return n;
    }
    return n->m_parseSource;
}

RVA(0x0013a330, 0xce)
CSymTab* CSymTab::CreateSub(const char* name) {

    if (m_subTabs.Walk(name, m_owner->m_caseSensitive == 0) != NULL) {
        return 0;
    }
    CSymTab* child = new CSymTab(
        m_owner,
        this,
        name,
        0,
        0,
        m_owner->MakeTimestamp(),
        m_owner->m_subTabBucketCount,
        m_owner->m_symbolBucketCount
    );
    if (!child) {
        return 0;
    }
    m_subTabs.Insert(&child->m_node20);

    u32 len = strlen(name);
    if (m_owner->m_longestScopeNameLen <= len) {
        m_owner->m_longestScopeNameLen = len + 1;
    }
    return child;
}

// @early-stop
RVA(0x0013a400, 0xa9)
CParseSource* CSymTab::AddNamedValue(void* unused, void* name, i32 key) {
    CSymRec* rec = FindOrAddSym(key);
    if (rec->m_valTable.Walk(static_cast<const char*>(name), m_owner->m_caseSensitive == 0)
        != NULL) {
        return 0;
    }
    CParseSource* slot = m_owner->PopParseSlot();
    slot->Build(
        this,
        static_cast<const char*>(name),
        &rec->m_valTable,
        rec,
        0,
        0,
        0,
        m_owner->MakeTimestamp(),
        0,
        0,
        m_owner->m_activeNode
    );
    if (slot == NULL) {
        return 0;
    }
    rec->m_valTable.Insert(&slot->m_node1c);
    u32 len = strlen(static_cast<char*>(name));
    if (m_owner->m_longestLeafNameLen <= len) {
        m_owner->m_longestLeafNameLen = len + 1;
    }
    return slot;
}

RVA(0x0013a4b0, 0x75)
CParseSource* CSymTab::AddNodeEntry(u32 key, const char* name, CSymRec* rec, CRezItmBase* stream) {
    CParseSource* slot = m_owner->PopParseSlot();
    if (slot == NULL) {
        return slot;
    }
    slot->Build(
        this,
        name,

        // API-forced pointer-key boundary.
        reinterpret_cast<void*>(key),
        rec,
        0,
        0,
        0,
        m_owner->MakeTimestamp(),
        0,
        0,
        stream
    );
    rec->m_valTable.Insert(&slot->m_node1c);
    u32 len = strlen(name);
    if (m_owner->m_longestLeafNameLen <= len) {
        m_owner->m_longestLeafNameLen = len + 1;
    }
    return slot;
}

RVA(0x0013a530, 0x47)
i32 CSymTab::AddNodeSubEntry(void* rec, void* found) {
    CParseSource* src = static_cast<CParseSource*>(found);
    m_totalSourceLength -= static_cast<i32>(src->m_length);
    (static_cast<CSymRec*>(rec))->m_valTable.Remove(&src->m_node1c);
    src->Teardown();
    m_owner->AddNode(found);
    m_owner->m_sorted = 0;
    return 1;
}

// @early-stop
RVA(0x0013a580, 0xb2)
i32 CSymTab::ApplyRecursive(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates) {
    i32 ok = 1;
    if (static_cast<u32>(dataSize) > 0) {
        CHashElement* e = m_subTabs.First();
        while (e) {
            e->m_symTab->m_dataOff = 0;
            e = e->Next();
        }
        if (ApplyRange(stream, dataOff, dataSize, mergeDuplicates) != 0) {
            e = m_subTabs.First();
            while (e) {
                CSymTab* sub = e->m_symTab;
                if (sub->m_dataOff != 0) {
                    if (sub->ApplyRecursive(
                            stream,
                            sub->m_dataOff,
                            sub->m_dataSize,
                            mergeDuplicates
                        )
                        == 0) {
                        ok = 0;
                    }
                }
                e = e->Next();
            }
        } else {
            ok = 0;
        }
    }
    return ok;
}

RVA(0x0013a640, 0x2f7)
i32 CSymTab::ApplyRange(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates) {
    m_totalSourceLength = 0;
    m_baseOffset = -1;
    i32 maxVal = 0;
    char* buf = new char[static_cast<u32>(dataSize)];
    if (!buf) {
        return 0;
    }
    if (stream->Read(dataOff, 0, dataSize, buf) != dataSize) {
        delete[] buf;
        return 0;
    }
    char* p = buf;
    char* end = buf + dataSize;
    while (p < end) {
        if (PeekI32(p) == 1) {
            p += 4;
            i32 fA = PeekI32(p);
            p += 4;
            i32 fB = PeekI32(p);
            p += 4;
            i32 fC = PeekI32(p);
            p += 4;
            char* name = p;
            p += strlen(name) + 1;
            CHashB* tabs = &m_subTabs;
            void* existing = tabs->Walk(name, m_owner->m_caseSensitive == 0);
            if (existing == NULL) {
                CSymTab* node = new CSymTab(
                    m_owner,
                    this,
                    name,
                    fA,
                    fB,
                    fC,
                    m_owner->m_subTabBucketCount,
                    m_owner->m_symbolBucketCount
                );
                tabs->Insert(&node->m_node20);
            } else {
                (static_cast<CSymTab*>(existing))->m_dataOff = fA;
                (static_cast<CSymTab*>(existing))->m_dataSize = fB;
                (static_cast<CSymTab*>(existing))->m_dirTime = fC;
            }
        } else {

            p += 4;
            i32 f1 = PeekI32(p);
            p += 4;
            i32 f3 = PeekI32(p);
            p += 4;
            i32 f2 = PeekI32(p);
            p += 4;
            i32 f4 = PeekI32(p);
            p += 4;
            i32 f5 = PeekI32(p);
            p += 4;
            i32 f6 = PeekI32(p);
            p += 4;
            char* name1 = p;
            p += strlen(name1) + 1;
            CSymRec* rec = FindOrAddSym(f5);
            i32 skip = 0;
            void* found = rec->m_valTable.Walk(name1, 1);
            if (found) {
                if (mergeDuplicates != 0) {
                    AddNodeSubEntry(rec, found);
                } else {
                    skip = 1;
                }
            }
            char* str2 = p;
            p += strlen(p) + 1;
            if (*str2 == 0) {
                str2 = NULL;
            }
            i32* arr;
            if (static_cast<u32>(f6) > 0) {
                arr = new i32[f6];
                for (u32 i = 0; i < static_cast<u32>(f6); i++) {
                    arr[i] = PeekI32(p);
                    p += 4;
                }
            } else {
                arr = NULL;
            }
            if (!skip) {
                CParseSource* slot = m_owner->PopParseSlot();
                AddrWord<char> entry;
                entry.m_word = f4;
                slot->Build(this, name1, entry.m_addr, rec, str2, f3, f1, f2, f6, arr, stream);
                rec->m_valTable.Insert(&slot->m_node1c);
                m_totalSourceLength = m_totalSourceLength + slot->m_length;
                if (static_cast<u32>(slot->m_base) < static_cast<u32>(m_baseOffset)) {
                    m_baseOffset = slot->m_base;
                }
                if (static_cast<u32>(slot->m_base) > static_cast<u32>(maxVal)) {
                    maxVal = slot->m_base;
                }
            }
            if (arr) {
                ::operator delete(arr);
            }
        }
    }
    delete[] buf;
    return 1;
}

RVA(0x0013a940, 0xc2)
CSymRec* CSymTab::FindOrAddSym(i32 key) {

    CSymRec* rec = static_cast<CSymRec*>(m_symbols.FindInt(static_cast<u32>(key)));
    if (!rec) {
        if (m_owner->m_useKeyIndex != 0) {
            rec = new CSymRec(key, this, m_owner->m_keyBucketCount, m_owner->m_valueBucketCount);
        } else {
            rec = new CSymRec(key, this, m_owner->m_valueBucketCount);
        }
        if (rec == NULL) {
            return 0;
        }
        m_symbols.Insert(&rec->m_symNode);
    }
    return rec;
}

RVA(0x0013aa10, 0xdc)
CSymParser::CSymParser() : m_hash(1) {
    m_parseArmed = 0;
    m_activeNode = NULL;
    m_list.m_count = 0;
    m_rootDataOffset = 0;
    m_nextWritePos = 0;
    m_root = NULL;
    m_archiveTime = 0;
    m_newArchive = false;
    m_version = 0;
    m_largestKeyArraySize = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_largestCommentSize = 0;
    m_cachedSourceBuffer = NULL;
    m_delims = NULL;
    m_caseSensitive = 0;
    m_useKeyIndex = 0;
    m_valueBucketCount = 0x13;
    m_keyBucketCount = 0x13;
    m_reserved24 = 1;
    m_nextGeneratedFileKey = 0x77359400;
    m_readOnly = 1;
    m_sorted = 1;
    m_maxOpenFiles = 3;
    m_subTabBucketCount = 5;
    m_symbolBucketCount = 9;
    m_parseSlotBlockCount = 0x64;
}

RVA_COMPGEN(0x0013aaf0, 0x7, ??1CParserObjList@@QAE@XZ)

RVA(0x0013ab00, 0xac)
CSymParser::CSymParser(void* buf, i32 a2, i32 a3) : m_hash(1) {
    {
        CSymParser tmp;
    }
    ParseBuffer(buf, a2, a3);
}

// CSymParser::m_nodes (this+0x88) - an empty inline destructor, so a lone `c3`.
RVA_COMPGEN(0x0013abb0, 0x1, ??1CSlotNodeList@@QAE@XZ)

RVA(0x0013abc0, 0x13f)
CSymParser::~CSymParser() {

    if (m_parseArmed) {
        Clear(0);
    }
    CRezItmBase* p;
    for (p = m_list.m_head; p != NULL; p = m_list.m_head) {
        m_list.Remove(p);
        m_list.m_count--;
        delete p;
    }
    CSymTab* root = m_root;
    if (root) {
        delete root;
        m_root = NULL;
    }
    if (m_cachedSourceBuffer) {
        delete[] m_cachedSourceBuffer;
        m_cachedSourceBuffer = NULL;
    }
    if (m_delims) {
        delete[] m_delims;
        m_delims = NULL;
    }
    CSlotNode* node = HeadSlotNode(m_nodes);
    m_parseArmed = 0;
    m_activeNode = NULL;
    m_rootDataOffset = 0;
    m_rootDataSize = 0;
    m_rootDirTime = 0;
    m_nextWritePos = 0;
    m_readOnly = 1;
    m_root = NULL;
    m_archiveTime = 0;
    m_newArchive = false;
    m_version = 1;
    m_largestKeyArraySize = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_largestCommentSize = 0;
    m_sorted = 1;
    m_cachedSourceBuffer = NULL;
    if (node) {
        do {
            delete[] node->m_buffer;
            m_nodes.Unlink(node);
            ::operator delete(node);
            node = HeadSlotNode(m_nodes);
        } while (node);
    }
}

// @early-stop
// 300/300 instructions at 952/952 bytes with every block boundary on retail's offset;
// the residue is 18 register NAMES in arm 1 (cl reuses the freed edi as the m_count
// scratch where retail keeps eax/ecx/edx). 472 variants found nothing.
RVA(0x0013ad00, 0x3b8)
i32 CSymParser::ParseBuffer(void* buf, i32 a, i32 b) {
    m_readOnly = a;
    if (a == 0) {
        return 0;
    }
    if (m_cachedSourceBuffer) {
        delete[] m_cachedSourceBuffer;
    }
    char* src = new char[strlen(static_cast<char*>(buf)) + 1];
    m_cachedSourceBuffer = src;
    strcpy(src, static_cast<char*>(buf));
    if (Classify(static_cast<char*>(buf)) != 0) {

        if (m_readOnly == 0) {
            return 0;
        }
        CRezItmBase* reader = new CRezDir(this, m_maxOpenFiles);
        if (reader == NULL) {
            delete[] m_cachedSourceBuffer;
            m_cachedSourceBuffer = NULL;
            return 0;
        }
        m_activeNode = reader;
        m_list.AddHead(reader);
        m_list.m_count++;
        if (reader->Open(static_cast<char*>(buf), a, b) == 0) {
            return 0;
        }
        m_parseArmed = 1;
        CSymTab* node = new CSymTab(
            this,
            0,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        ParseRecords(reader, node, m_cachedSourceBuffer, 0);
        return 1;
    }

    CRezItmBase* reader = new CRezItm(this);
    if (reader == NULL) {
        delete[] m_cachedSourceBuffer;
        m_cachedSourceBuffer = NULL;
        return 0;
    }
    m_activeNode = reader;
    m_list.AddHead(reader);
    m_list.m_count++;
    if (reader->Open(static_cast<char*>(buf), a, b) == 0) {
        return 0;
    }
    m_parseArmed = 1;
    if (b != 0) {
        m_nextWritePos = sizeof(SymTabFileHeader);
        m_newArchive = true;
        CSymTab* node = new CSymTab(
            this,
            0,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        return 1;
    }

    SymTabFileHeader hdr;
    reader->Read(0, 0, 0xa8, &hdr);
    m_version = hdr.m_version;
    m_rootDataOffset = hdr.m_rootDataOffset;
    m_rootDataSize = hdr.m_rootDataSize;
    m_rootDirTime = hdr.m_rootDirTime;
    m_nextWritePos = hdr.m_nextWritePos;
    m_archiveTime = hdr.m_archiveTime;
    m_largestKeyArraySize = hdr.m_largestKeyArraySize;
    m_longestScopeNameLen = hdr.m_longestScopeNameLen;
    m_longestLeafNameLen = hdr.m_longestLeafNameLen;
    m_largestCommentSize = hdr.m_largestCommentSize;
    m_sorted = hdr.m_sorted;
    // Four separate guards: retail emits four inline `xor eax,eax; jmp ret` blocks,
    // not the single short-circuit target a `||` chain produces.
    if (hdr.m_magic0 != SYMTAB_MAGIC_CR) {
        return 0;
    }
    if (hdr.m_magic3f != SYMTAB_MAGIC_LF) {
        return 0;
    }
    if (hdr.m_magic7e != SYMTAB_MAGIC_EOF) {
        return 0;
    }
    if (hdr.m_version != 1) {
        return 0;
    }
    CSymTab* node = new CSymTab(
        this,
        0,
        "",
        m_rootDataOffset,
        m_rootDataSize,
        m_rootDirTime,
        m_subTabBucketCount,
        m_symbolBucketCount
    );
    m_root = node;
    node->ApplyRecursive(reader, m_rootDataOffset, m_rootDataSize, 0);
    return 1;
}

// @early-stop
RVA(0x0013b0c0, 0x238)
i32 CSymParser::LoadEntry(char* name, i32 flag) {
    if (m_readOnly == 0) {
        return 0;
    }
    m_sorted = 0;
    if (m_cachedSourceBuffer) {
        delete[] m_cachedSourceBuffer;
    }
    char* buf = new char[strlen(name) + 1];
    m_cachedSourceBuffer = buf;
    strcpy(buf, name);

    if (Classify(name)) {
        CRezItmBase* node = new CRezDir(this, m_maxOpenFiles);
        if (node == NULL) {
            delete[] m_cachedSourceBuffer;
            m_cachedSourceBuffer = NULL;
            return 0;
        }
        m_list.AddHead(node);
        m_list.m_count++;
        if (node->Open(name, 1, 0) == 0) {
            return 0;
        }
        m_parseArmed = 1;
        ParseRecords(node, m_root, m_cachedSourceBuffer, flag);
        return 1;
    }

    CRezItmBase* node = new CRezItm(this);
    if (node == NULL) {
        delete[] m_cachedSourceBuffer;
        m_cachedSourceBuffer = NULL;
        return 0;
    }
    m_list.AddHead(node);
    m_list.m_count++;
    if (node->Open(name, 1, 0) == 0) {
        return 0;
    }

    SymTabFileHeader hdr;
    node->Read(0, 0, 0xa8, &hdr);
    u32 v;
    v = hdr.m_largestKeyArraySize;
    if (v > m_largestKeyArraySize) {
        m_largestKeyArraySize = v;
    }
    v = hdr.m_longestScopeNameLen;
    if (v > m_longestScopeNameLen) {
        m_longestScopeNameLen = v;
    }
    v = hdr.m_longestLeafNameLen;
    if (v > m_longestLeafNameLen) {
        m_longestLeafNameLen = v;
    }
    v = hdr.m_largestCommentSize;
    if (v > m_largestCommentSize) {
        m_largestCommentSize = v;
    }
    m_root->ApplyRecursive(node, hdr.m_rootDataOffset, hdr.m_rootDataSize, flag);
    return 1;
}

// @early-stop
// Instruction stream and frame layout are identical to retail; the residue is one
// callee-saved colour swap (extKey in esi / rec in edi, ours reversed) around the
// PackTag/AddNodeEntry block.
RVA(0x0013b300, 0x545)
i32 CSymParser::ParseRecords(void* reader, CSymTab* node, char* path, i32 flag) {
    char pattern[REZ_SCAN_PATH_MAX];
    strcpy(pattern, path);
    if (pattern[strlen(pattern) - 1] != '\\') {
        strcat(pattern, g_sepSlash);
    }
    char full[REZ_SCAN_PATH_MAX];
    strcpy(full, pattern);
    strcat(full, g_wildcard);
    _finddata_t fd;
    i32 h = _findfirst(full, &fd);
    if (h < 0) {
        return 1;
    }
    do {
        if (strcmp(fd.name, g_dot) == 0 || strcmp(fd.name, g_dotDot) == 0) {
            continue;
        }
        if ((fd.attrib & _A_SUBDIR) == _A_SUBDIR) {

            char subName[REZ_SCAN_PATH_MAX];
            strcpy(subName, fd.name);
            if (m_caseSensitive == 0) {
                _strupr(subName);
            }
            char childpath[REZ_SCAN_PATH_MAX];
            strcpy(childpath, pattern);
            strcat(childpath, subName);
            strcat(childpath, g_sepSlash);
            void* child = node->FindSub(subName);
            if (child == NULL) {
                child = node->CreateSub(subName);
                if (child == NULL) {
                    continue;
                }
            }
            ParseRecords(reader, static_cast<CSymTab*>(child), childpath, flag);
            continue;
        }

        char filepath[REZ_SCAN_PATH_MAX];
        strcpy(filepath, pattern);
        strcat(filepath, fd.name);
        char drive[_MAX_DRIVE];
        char dir[_MAX_PATH];
        char splitName[_MAX_PATH];
        char fname[REZ_SCAN_PATH_MAX];
        char ext[_MAX_PATH];
        _splitpath(filepath, drive, dir, splitName, ext);
        strcpy(fname, splitName);
        _strupr(fname);
        i32 nleft = static_cast<i32>(strlen(fname));
        i32 i = 0;
        while (i < nleft && fname[i] >= '0' && fname[i] <= '9') {
            i++;
        }
        i32 key = (i < nleft) ? static_cast<i32>(m_nextGeneratedFileKey++) : atol(fname);
        RezTypeTag extKey;
        char extName[8];
        char unpackedTag[8];
        if (strlen(ext) != 0) {
            strcpy(extName, ext + 1);
            _strupr(extName);
            extKey = PackTag(extName);
        } else {
            extKey = REZ_TAG_NONE;
        }
        UnpackTag(extKey, unpackedTag);
        CSymRec* rec = node->FindOrAddSym(IDX(extKey));
        CParseSource* entry = node->Insert(fname, extKey);
        CParseSource* source;
        if (entry == NULL) {
            source = node->AddNodeEntry(static_cast<u32>(key), fname, rec, 0);
        } else if (flag != 0) {
            node->AddNodeSubEntry(rec, entry);
            source = node->AddNodeEntry(static_cast<u32>(key), fname, rec, 0);
        } else {
            source = NULL;
        }
        if (source != NULL) {
            source->m_typeTag = static_cast<i32>(fd.time_write);
            source->m_length = static_cast<u32>(fd.size);
            source->m_reader = new CRezFile(this, filepath, static_cast<CRezDir*>(reader));
        }
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
    return 1;
}

// @early-stop
// One transposition: retail stores the cleared m_activeNode BEFORE reading the list
// head, cl after. The loop spelling is not the lever - for, while-assign and a
// loop-scoped cursor are byte-identical.
RVA(0x0013b850, 0xa8)
i32 CSymParser::Clear(i32 final) {
    static_cast<void>(final);
    i32 r = m_activeNode->Close();
    m_list.Remove(m_activeNode);
    m_list.m_count--;
    delete m_activeNode;
    m_activeNode = NULL;
    CRezItmBase* p;
    for (p = m_list.m_head; p != NULL; p = m_list.m_head) {
        p->Close();
        m_list.Remove(p);
        m_list.m_count--;
        delete p;
    }
    if (m_root) {
        delete m_root;
        m_root = NULL;
    }
    if (m_cachedSourceBuffer) {
        delete[] m_cachedSourceBuffer;
        m_cachedSourceBuffer = NULL;
    }
    m_parseArmed = 0;
    return r;
}

RVA(0x0013b900, 0x4)
CSymTab* CSymParser::GetRoot() {
    return m_root;
}

RVA(0x0013b910, 0x58)
RezTypeTag CSymParser::PackTag(const char* s) {
    if (!s) {
        return REZ_TAG_NONE;
    }
    DwordBytes r;
    r.m_value = 0;
    u8* rb = r.m_bytes;
    i32 len = static_cast<i32>(strlen(s));
    if (len > 0) {
        rb[len - 1] = s[0];
    }
    if (len > 1) {
        rb[len - 2] = s[1];
    }
    if (len > 2) {
        rb[len - 3] = s[2];
    }
    if (len > 3) {
        rb[len - 4] = s[3];
    }
    return static_cast<RezTypeTag>(r.m_value);
}

RVA(0x0013b970, 0x72)
void __stdcall UnpackTag(RezTypeTag tag, char* dst) {
    if (!dst) {
        return;
    }
    // The tag's four characters, most significant byte first. Byte-evidenced: retail
    // addresses the PARAMETER's own slot (`mov cx,[esp+6]`, `mov cl,[esp+eax+3]`), so
    // this is a view of `tag`, never the copy a union local would make.
    const u8* tb = static_cast<const u8*>(static_cast<const void*>(&tag));
    i32 len = 0;
    if (tb[3]) {
        len = 4;
    } else if (tb[2]) {
        len = 3;
    } else if (tb[1]) {
        len = 2;
    } else if (tb[0]) {
        len = 1;
    }
    if (len > 0) {
        dst[0] = tb[len - 1];
    }
    if (len > 1) {
        dst[1] = tb[len - 2];
    }
    if (len > 2) {
        dst[2] = tb[len - 3];
    }
    if (len > 3) {
        dst[3] = tb[len - 4];
    }
    dst[len] = 0;
}

RVA(0x0013b9f0, 0x5)
i32 CSymParser::UnusedParserQuery(i32 a) {
    return 0;
}

RVA(0x0013ba00, 0x3)
void CSymParser::UnusedParserAction(i32 a) {}

RVA(0x0013ba10, 0x3)
i32 CSymParser::Retry() {
    return 0;
}

RVA(0x0013ba20, 0x27)
i32 CSymParser::CheckNodes() {
    i32 ok = 1;
    for (CRezItmBase* n = m_list.m_head; n != NULL; n = n->m_next) {
        if (n->Check() == 0) {
            ok = 0;
        }
    }
    return ok;
}

RVA(0x0013ba50, 0x1f)
void CSymParser::SetBucketCounts(
    i32 valueBuckets,
    i32 keyBuckets,
    i32 subTabBuckets,
    i32 symbolBuckets
) {
    m_valueBucketCount = valueBuckets;
    m_keyBucketCount = keyBuckets;
    m_subTabBucketCount = subTabBuckets;
    m_symbolBucketCount = symbolBuckets;
}

RVA(0x0013ba70, 0x10)
i32 CSymParser::MakeTimestamp() {
    time_t t;
    return static_cast<i32>(time(&t));
}

RVA(0x0013ba80, 0x57)
void CSymParser::SetDelims(char* s) {
    if (m_delims != NULL) {
        delete[] m_delims;
    }
    m_delims = new char[strlen(s) + 1];
    strcpy(m_delims, s);
}

RVA(0x0013bae0, 0x1b9)
void* CSymTab::ResolvePath(const char* path) {
    char buf[0x40];
    if (static_cast<i32>(strlen(path)) > 1) {
        if (!IsTokenChar(m_owner->m_delims, *path)) {
            ++path;
        }
    }
    const char* p = path;
    i32 n = 0;
    while (IsTokenChar(m_owner->m_delims, *p)) {
        buf[n] = *p;
        ++n;
        ++p;
    }
    buf[n] = 0;
    void* sub = FindSub(buf);
    if (!sub) {
        return sub;
    }
    char c = path[n];
    if (c == 0) {
        return sub;
    }
    while (!IsTokenChar(m_owner->m_delims, c)) {
        c = path[n + 1];
        ++n;
        if (c == 0) {
            return sub;
        }
    }
    return (static_cast<CSymTab*>(sub))->ResolvePath(path + n);
}

RVA(0x0013bca0, 0x19c)
void* CSymTab::FindQualified(const char* name) {
    char path[0x100];
    char leaf[0x20];
    i32 len = static_cast<i32>(strlen(name));
    if (len > 1) {
        if (!IsTokenChar(m_owner->m_delims, *name)) {
            ++name;
            --len;
        }
    }
    i32 i = len - 1;
    while (IsTokenChar(m_owner->m_delims, name[i])) {
        --i;
        if (i < 0) {
            break;
        }
    }
    if (i == len) {
        return 0;
    }
    const char* tail = name + i + 1;
    strcpy(leaf, tail);
    if (i <= 1) {
        return Find(leaf);
    }
    strncpy(path, name, static_cast<u32>(i));
    path[i] = 0;
    CSymTab* scope = static_cast<CSymTab*>(ResolvePath(path));
    if (!scope) {
        return 0;
    }
    return scope->Find(leaf);
}

RVA(0x0013be40, 0x1ac)
CParseSource* CSymTab::ResolveQualified(const char* name, RezTypeTag fourcc) {
    char path[0x100];
    char leaf[0x20];
    i32 len = static_cast<i32>(strlen(name));
    if (len > 1) {
        if (!IsTokenChar(m_owner->m_delims, *name)) {
            ++name;
            --len;
        }
    }
    i32 i = len - 1;
    while (IsTokenChar(m_owner->m_delims, name[i])) {
        --i;
        if (i < 0) {
            break;
        }
    }
    if (i == len) {
        return 0;
    }
    const char* tail = name + i + 1;
    strcpy(leaf, tail);
    if (i <= 1) {
        return Insert(leaf, fourcc);
    }
    strncpy(path, name, static_cast<u32>(i));
    path[i] = 0;
    CSymTab* scope = static_cast<CSymTab*>(ResolvePath(path));
    if (!scope) {
        return 0;
    }
    return scope->Insert(leaf, fourcc);
}

RVA(0x0013bff0, 0x19)
CParseSource* CSymParser::ResolveQualified(const char* name, RezTypeTag fourcc) {
    return GetRoot()->ResolveQualified(name, fourcc);
}

RVA(0x0013c010, 0x14)
void* CSymParser::FindQualified(const char* name) {
    return GetRoot()->FindQualified(name);
}

RVA(0x0013c030, 0x14)
void* CSymParser::ResolvePath(const char* path) {
    return GetRoot()->ResolvePath(path);
}

RVA(0x0013c050, 0x28)
i32 CSymParser::ReParse() {
    if (m_parseArmed == 0) {
        return 0;
    }
    Clear(0);
    return ParseBuffer(m_cachedSourceBuffer, 1, 0);
}

RVA(0x0013c080, 0x3c)
i32 CSymParser::Classify(char* name) {
    struct _stat rec;
    if (_stat(name, &rec) != 0) {
        return 0;
    }
    return (rec.st_mode & _S_IFDIR) == _S_IFDIR;
}

// @early-stop
RVA(0x0013c0c0, 0x14b)
CParseSource* CSymParser::PopParseSlot() {
    void* rec = 0;
    CHashElement* e = m_hash.First();
    if (e != NULL) {
        rec = e->m_parseSource;
    }
    if (rec == NULL) {
        CSlotNode* node = new CSlotNode;
        if (node == NULL) {
            return 0;
        }
        CParseSource* arr = new CParseSource[m_parseSlotBlockCount];
        node->m_buffer = arr;
        if (arr == NULL) {
            ::operator delete(node);
            return 0;
        }
        for (i32 j = 0; static_cast<u32>(j) < static_cast<u32>(m_parseSlotBlockCount); j++) {
            node->m_buffer[j].m_node1c.m_parseSource = &node->m_buffer[j];
            m_hash.Insert(&node->m_buffer[j].m_node1c);
        }
        m_nodes.InsertHead(node);
        e = m_hash.First();
        rec = e->m_parseSource;
    }
    if (rec) {
        m_hash.Remove(&(static_cast<CParseSource*>(rec))->m_node1c);
    }
    return static_cast<CParseSource*>(rec);
}

RVA(0x0013c210, 0x1a)
void CSymParser::AddNode(void* rec) {
    if (rec) {
        m_hash.Insert(&(static_cast<CParseSource*>(rec))->m_node1c);
    }
}
