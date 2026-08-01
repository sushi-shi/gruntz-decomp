

#include <Rez/DebugPrintf.h>
#include <Bute/SymTab.h>
#include <AddrWord.h>
#include <Mfc.h>
#include <rva.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Bute/SymParser.h>
#include <Gruntz/ParseSource.h>
#include <Rez/RezMgr.h>
#include <Rez/RezFile.h>

#include <Dsndmgr/SoundBankLoad.h>
#include <Gruntz/CustomWorldInfoDlg.h>
VTBL(CParseSlotHashNode, 0x001ef740);
VTBL(CSymRecNode, 0x001ef744);
VTBL(CSymTabNode, 0x001ef748);
VTBL(CSymParser, 0x001ef750);
VTBL(CParserObjList, 0x001ef75c);
DATA(0x0020cff0)
const char g_sepSlash[] = "\\";

inline void* operator new(u32, void* p) {
    return p;
}

static __inline i32 IsTokenChar(const char* delims, char ch) {
    if (delims) {
        return strchr(delims, ch) == 0;
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

// Byte-forced view of packed serialized storage.
static inline i32 PeekI32(const char* p) {
    return *reinterpret_cast<const i32*>(p);
}

RVA(0x001396f0, 0x1a)
CParseSource::CParseSource() {

    m_reader = 0;
    m_owner = 0;
    m_name = 0;
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
    if (name == 0) {
        m_name = const_cast<char*>(name);
    } else {
        m_name = static_cast<char*>(::operator new(strlen(name) + 1));
        if (m_name) {
            strcpy(m_name, name);
        }
    }
    m_entry = rec;
    m_length = f3;
    m_base = f1;
    m_typeTag = f2;
    m_buffer = 0;
    m_cursor = 0;
    m_node1c.m_parseSource = this;
}

RVA(0x001397a0, 0x57)
void CParseSource::Teardown() {
    if (m_name) {
        ::operator delete(m_name);
    }
    if (m_owner != 0) {
        if (m_owner->m_mappedBuf == 0) {
            if (m_buffer) {
                ::operator delete(m_buffer);
            }
        }
    } else {
        if (m_buffer) {
            ::operator delete(m_buffer);
        }
    }
    m_name = 0;
    m_entry = 0;
    m_typeTag = 0;
    m_length = 0;
    m_buffer = 0;
    m_owner = 0;
    m_base = 0;
    m_cursor = 0;
    m_node1c.m_parseSource = 0;
}

RVA(0x00139800, 0x6)
i32 CParseSource::GetEntryTag() {
    return m_entry->m_key;
}

RVA(0x00139810, 0x140)
char* CParseSource::CurrentScopePath(char* dst, i32 size) {
    if (m_owner->m_parent == 0) {
        strcpy(dst, g_sepSlash);
    } else {
        char* scratch = static_cast<char*>(::operator new(size));
        strcpy(dst, g_emptyString);
        CSymTab* node = m_owner;
        while (node != 0) {
            strcpy(scratch, dst);
            if (node->m_parent != 0) {
                strcpy(dst, g_sepSlash);
            } else {
                dst[0] = 0;
            }
            strcat(dst, node->m_name);
            strcat(dst, scratch);
            node = node->m_parent;
        }
        ::operator delete(scratch);
    }
    return dst;
}

RVA(0x00139950, 0x6)
char* CParseSource::CurrentScopeName() {
    return m_owner->m_name;
}

RVA(0x00139960, 0x6b)
char* CParseSource::BeginParse() {
    if (m_owner->m_mappedBuf != 0) {
        return m_owner->m_mappedBuf + (m_base - m_owner->m_baseOffset);
    }
    if (m_buffer != 0) {
        return m_buffer;
    }
    if (m_length == 0) {
        return 0;
    }
    m_buffer = static_cast<char*>(RezAlloc(m_length));
    if (m_buffer == 0) {
        return 0;
    }
    if (m_reader->Read(m_base, 0, m_length, m_buffer) != static_cast<i32>(m_length)) {
        delete m_buffer;
        m_buffer = 0;
    }
    return m_buffer;
}

RVA(0x001399d0, 0x21)
i32 CParseSource::EndParse() {
    if (m_buffer != 0) {
        ::operator delete(m_buffer);
        m_buffer = 0;
    }
    return 1;
}

RVA(0x00139a40, 0x95)
i32 CParseSource::ReadAt(void* dst, i32 pos, u32 len) {
    CSymTab* sd = m_owner;
    if (sd->m_mappedBuf != 0) {
        memcpy(dst, sd->m_mappedBuf + (m_base - sd->m_baseOffset + pos), len);
        return 1;
    }
    if (m_buffer != 0) {
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

// @early-stop
RVA(0x00139bf0, 0x71)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c, i32 d) : m_keyTable(c), m_valTable(d) {
    m_symNode.m_symRec = this;
    m_scope = owner;
    m_key = key;
}

RVA(0x00139c80, 0x6c)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c) : m_keyTable(), m_valTable(c) {
    m_key = key;
    m_symNode.m_symRec = this;
    m_scope = owner;
}

RVA(0x00139cf0, 0xd7)
CSymRec::~CSymRec() {
    if (m_scope->m_owner->m_6c != 0) {
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
    m_symNode.m_symRec = 0;
}

// @early-stop
RVA(0x00139de0, 0xd4)
CSymTab::CSymTab(
    CSymParser* owner,
    CSymTab* parent,
    const char* name,
    i32 dataOff,
    i32 dataSize,
    i32 seed,
    i32 subN,
    i32 symN
)
    : m_subTabs(subN), m_symbols(symN) {
    m_name = static_cast<char*>(::operator new(strlen(name) + 1));
    if (m_name) {
        strcpy(m_name, name);
    }
    m_seed = seed;
    m_dataSize = dataSize;
    m_dataOff = dataOff;
    m_owner = owner;
    m_10 = 0;
    m_baseOffset = 0;
    m_mappedBuf = 0;
    m_parent = parent;
    m_node20.m_symTab = this;
}

RVA(0x00139ee0, 0x11e)
CSymTab::~CSymTab() {

    CHashElement* e;
    for (e = m_symbols.First(); e != 0;) {
        CHashElement* cur = e;
        e = cur->Next();
        m_symbols.Remove(cur);
        CSymRec* rec = cur->m_symRec;
        delete rec;
    }
    for (e = m_subTabs.First(); e != 0;) {
        CHashElement* cur = e;
        e = cur->Next();
        m_subTabs.Remove(cur);
        CSymTab* sub = cur->m_symTab;
        delete sub;
    }
    if (m_name) {
        ::operator delete(m_name);
    }
    if (m_mappedBuf) {
        ::operator delete(m_mappedBuf);
    }
    m_name = 0;
    m_seed = 0;
    m_dataSize = 0;
    m_dataOff = 0;
    m_10 = 0;
    m_baseOffset = 0;
    m_mappedBuf = 0;
    m_owner = 0;
    m_parent = 0;
    m_node20.m_symTab = 0;
}

RVA(0x0013a000, 0x37)
CParseSource* CSymTab::Insert(const char* key, u32 fourcc) {
    CSymRec* rec = static_cast<CSymRec*>(m_symbols.FindInt(fourcc));
    if (!rec) {
        return 0;
    }
    return static_cast<CParseSource*>(rec->m_valTable.Walk(key, m_owner->m_68 == 0));
}

RVA(0x0013a040, 0xa2)
void* CSymTab::Find(const char* path) {
    char dir[260];
    char fname[260];
    char ext[260];
    char drive[4];
    char tmp[8];
    _splitpath(path, drive, dir, fname, ext);
    u32 fourcc;
    if (strlen(ext) != 0) {
        strcpy(tmp, ext + 1);
        _strupr(tmp);
        fourcc = static_cast<u32>(m_owner->PackTag(tmp));
    } else {
        fourcc = 0;
    }
    return Insert(fname, fourcc);
}

RVA(0x0013a0f0, 0x99)
i32 CRezDirNode::Load(i32 childFlag) {
    if (m_buf != 0) {
        return 1;
    }

    RezSrc* src = m_src;
    if (src->m_8 == 0 || src->m_1c > 1) {
        RezAssertFail("CRezDir::Load Failed! (File is not sorted!)");
        return 0;
    }

    if (m_size > 0) {
        m_buf = static_cast<u8*>(RezAlloc(m_size));
        if (m_buf != 0) {
            m_src->m_stream->Read(m_off, 0, m_size, m_buf);
        }
    }

    if (childFlag != 0) {
        for (CHashElement* n = m_kids.First(); n != 0; n = n->Next()) {

            n->m_rezDirNode->Load(1);
        }
    }
    return 1;
}

RVA(0x0013a190, 0x94)
i32 CSymTab::ReleaseParseBuffers(i32 recurse) {
    if (m_mappedBuf != 0) {
        ::operator delete(m_mappedBuf);
        m_mappedBuf = 0;
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
    return m_subTabs.Walk(name, m_owner->m_68 == 0);
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

    if (m_subTabs.Walk(name, m_owner->m_68 == 0) != 0) {
        return 0;
    }
    CSymTab* child = new CSymTab(
        m_owner,
        this,
        name,
        0,
        0,
        m_owner->MakeSeed(),
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
    if (rec->m_valTable.Walk(static_cast<const char*>(name), m_owner->m_68 == 0) != 0) {
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
        m_owner->MakeSeed(),
        0,
        0,
        m_owner->m_activeNode
    );
    if (slot == 0) {
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
    if (slot == 0) {
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
        m_owner->MakeSeed(),
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
    m_10 -= static_cast<i32>(src->m_length);
    (static_cast<CSymRec*>(rec))->m_valTable.Remove(&src->m_node1c);
    src->Teardown();
    m_owner->AddNode(found);
    m_owner->m_08 = 0;
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

// @early-stop
RVA(0x0013a640, 0x2f7)
i32 CSymTab::ApplyRange(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates) {
    m_10 = 0;
    m_baseOffset = -1;
    i32 maxVal = 0;
    char* buf = static_cast<char*>(::operator new(static_cast<u32>(dataSize)));
    if (!buf) {
        return 0;
    }
    if (stream->Read(dataOff, 0, dataSize, buf) != dataSize) {
        ::operator delete(buf);
        return 0;
    }
    char* p = buf;
    char* end = buf + dataSize;
    while (p < end) {
        if (PeekI32(p) == 1) {

            i32 fA = PeekI32(p + 4);
            p += 8;
            i32 fB = PeekI32(p);
            i32 fC = PeekI32(p + 4);
            p += 8;
            char* name = p;
            p += strlen(name) + 1;
            void* existing = m_subTabs.Walk(name, m_owner->m_68 == 0);
            if (existing == 0) {
                CSymParser* o = m_owner;
                CSymTab* node = new CSymTab(
                    o,
                    this,
                    name,
                    fA,
                    fB,
                    fC,
                    o->m_subTabBucketCount,
                    o->m_symbolBucketCount
                );
                m_subTabs.Insert(&node->m_node20);
            } else {
                (static_cast<CSymTab*>(existing))->m_dataOff = fA;
                (static_cast<CSymTab*>(existing))->m_dataSize = fB;
                (static_cast<CSymTab*>(existing))->m_seed = fC;
            }
        } else {

            i32 f1 = PeekI32(p + 4);
            p += 8;
            i32 f3 = PeekI32(p);
            i32 f2 = PeekI32(p + 4);
            p += 8;
            i32 f4 = PeekI32(p);
            i32 f5 = PeekI32(p + 4);
            p += 8;
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
            if (*str2 == 0) {
                str2 = 0;
            }
            p += strlen(p) + 1;
            void* arr;
            if (f6 != 0) {
                arr = ::operator new(static_cast<u32>((f6 * 4)));
                for (i32 i = f6; i != 0; i--) {
                    *static_cast<i32*>(arr) = PeekI32(p);
                    arr = static_cast<char*>(arr) + 4;
                    p += 4;
                }
                arr = static_cast<char*>(arr) - f6 * 4;
            } else {
                arr = 0;
            }
            if (!skip) {
                CParseSource* slot = m_owner->PopParseSlot();
                AddrWord<char> entry;
                entry.m_word = f4;
                slot->Build(this, name1, entry.m_addr, rec, str2, f3, f1, f2, f6, arr, stream);
                rec->m_valTable.Insert(&slot->m_node1c);
                m_10 = m_10 + slot->m_length;
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
    ::operator delete(buf);
    return 1;
}

RVA(0x0013a940, 0xc2)
CSymRec* CSymTab::FindOrAddSym(i32 key) {

    CSymRec* rec = static_cast<CSymRec*>(m_symbols.FindInt(static_cast<u32>(key)));
    if (!rec) {
        if (m_owner->m_6c != 0) {
            rec = new CSymRec(key, this, m_owner->m_74, m_owner->m_70);
        } else {
            rec = new CSymRec(key, this, m_owner->m_70);
        }
        if (rec == 0) {
            return 0;
        }
        m_symbols.Insert(&rec->m_symNode);
    }
    return rec;
}

// @early-stop
RVA(0x0013aa10, 0xdc)
CSymParser::CSymParser() {
    m_list.m_head = 0;
    m_list.m_tail = 0;
    m_hash.Construct(1);
    m_parseArmed = 0;
    m_activeNode = 0;
    m_list.m_count = 0;
    m_30 = 0;
    m_3c = 0;
    m_root = 0;
    m_48 = 0;
    m_4c = 0;
    m_50 = 0;
    m_54 = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_60 = 0;
    m_cachedSourceBuffer = 0;
    m_delims = 0;
    m_68 = 0;
    m_6c = 0;
    m_70 = 0x13;
    m_74 = 0x13;
    m_24 = 1;
    m_40 = 1;
    m_nextGeneratedFileKey = 0x77359400;
    m_08 = 1;
    m_symbolBucketCount = 9;
    m_2c = 3;
    m_subTabBucketCount = 5;
    m_parseSlotBlockCount = 0x64;
}

RVA_COMPGEN(0x0013aaf0, 0x7, ??1CParserObjList@@QAE@XZ)

// @early-stop
RVA(0x0013ab00, 0xac)
CSymParser::CSymParser(void* buf, i32 a2, i32 a3) {

    m_list.m_head = 0;
    m_list.m_tail = 0;
    m_hash.Construct(1);
    {
        CSymParser tmp;
    }
    ParseBuffer(buf, a2, a3);
}

RVA(0x0013abc0, 0x13f)
CSymParser::~CSymParser() {

    if (m_parseArmed) {
        Clear(0);
    }
    CRezItmBase* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        m_list.Remove(p);
        m_list.m_count--;
        delete p;
    }
    CSymTab* root = m_root;
    if (root) {
        delete root;
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    if (m_delims) {
        ::operator delete(m_delims);
        m_delims = 0;
    }
    CSlotNode* node = HeadSlotNode(m_nodes);
    m_parseArmed = 0;
    m_activeNode = 0;
    m_30 = 0;
    m_34 = 0;
    m_38 = 0;
    m_3c = 0;
    m_40 = 1;
    m_root = 0;
    m_48 = 0;
    m_4c = 0;
    m_50 = 1;
    m_54 = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_60 = 0;
    m_08 = 1;
    m_cachedSourceBuffer = 0;
    if (node) {
        do {
            ::operator delete(node->m_buffer);
            m_nodes.Unlink(node);
            ::operator delete(node);
            node = HeadSlotNode(m_nodes);
        } while (node);
    }
}

// @early-stop
RVA(0x0013ad00, 0x3b8)
i32 CSymParser::ParseBuffer(void* buf, i32 a, i32 b) {
    m_40 = a;
    if (a == 0) {
        return 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
    }
    char* src = static_cast<char*>(::operator new(strlen(static_cast<char*>(buf)) + 1));
    m_cachedSourceBuffer = src;
    strcpy(src, static_cast<char*>(buf));
    i32 tag = Classify(static_cast<char*>(buf));
    if (tag != 0) {

        if (m_40 == 0) {
            return 0;
        }
        CRezItmBase* reader = new CRezDir(this, m_2c);
        if (reader == 0) {
            ::operator delete(m_cachedSourceBuffer);
            m_cachedSourceBuffer = 0;
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
            g_emptyString,
            0,
            0,
            this->MakeSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        ParseRecords(reader, node, m_cachedSourceBuffer, 0);
        return 1;
    }

    CRezItmBase* reader = new CRezItm(this);
    if (reader == 0) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
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
        m_3c = 0xa8;
        m_4c = 1;
        CSymTab* node = new CSymTab(
            this,
            0,
            g_emptyString,
            0,
            0,
            this->MakeSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        return 1;
    }

    SymTabFileHeader hdr;
    reader->Read(0, 0, 0xa8, &hdr);
    m_50 = hdr.m_flag;
    m_30 = hdr.m_scopeCount;
    m_34 = hdr.m_leafCount;
    m_38 = hdr.m_38;
    m_3c = hdr.m_3c;
    m_48 = hdr.m_48;
    m_54 = hdr.m_54;
    m_longestScopeNameLen = hdr.m_longestScopeNameLen;
    m_longestLeafNameLen = hdr.m_longestLeafNameLen;
    m_60 = hdr.m_60;
    m_08 = hdr.m_08;
    if (hdr.m_magic0 != 0x0d || hdr.m_magic3f != 0x0a || hdr.m_magic7e != 0x1a || b != 1) {
        return 0;
    }
    CSymTab* node = new CSymTab(
        this,
        0,
        g_emptyString,
        m_30,
        m_34,
        m_38,
        m_subTabBucketCount,
        m_symbolBucketCount
    );
    m_root = node;
    node->ApplyRecursive(reader, m_30, m_34, 0);
    return 1;
}

// @early-stop
RVA(0x0013b0c0, 0x238)
i32 CSymParser::LoadEntry(char* name, i32 flag) {
    if (m_40 == 0) {
        return 0;
    }
    m_08 = 0;
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
    }
    char* buf = static_cast<char*>(::operator new(strlen(name) + 1));
    m_cachedSourceBuffer = buf;
    strcpy(buf, name);

    if (Classify(name)) {
        CRezItmBase* node = new CRezDir(this, m_2c);
        if (node == 0) {
            ::operator delete(m_cachedSourceBuffer);
            m_cachedSourceBuffer = 0;
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
    if (node == 0) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
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
    v = hdr.m_54;
    if (v > m_54) {
        m_54 = v;
    }
    v = hdr.m_longestScopeNameLen;
    if (v > m_longestScopeNameLen) {
        m_longestScopeNameLen = v;
    }
    v = hdr.m_longestLeafNameLen;
    if (v > m_longestLeafNameLen) {
        m_longestLeafNameLen = v;
    }
    v = hdr.m_60;
    if (v > m_60) {
        m_60 = v;
    }
    m_root->ApplyRecursive(node, hdr.m_scopeCount, hdr.m_leafCount, flag);
    return 1;
}

// @early-stop
RVA(0x0013b300, 0x545)
i32 CSymParser::ParseRecords(void* reader, CSymTab* node, char* path, i32 flag) {
    char pattern[0x500];
    strcpy(pattern, path);
    if (pattern[strlen(pattern) - 1] != '\\') {
        strcpy(pattern + strlen(pattern), g_sepSlash);
    }
    char full[0x600];
    strcpy(full, pattern);
    strcpy(full + strlen(full), g_wildcard);
    _finddata_t fd;
    i32 h = _findfirst(full, &fd);
    if (h < 0) {
        return 1;
    }
    do {
        if (strcmp(fd.name, g_dot) == 0 || strcmp(fd.name, g_dotDot) == 0) {
            continue;
        }
        if ((fd.attrib & 0x10) == 0x10) {

            char childpath[0x600];
            strcpy(childpath, pattern);
            strcpy(childpath + strlen(childpath), fd.name);
            _strlwr(childpath);
            void* child = node->FindSub(fd.name);
            if (child == 0) {
                child = node->CreateSub(fd.name);
                if (child == 0) {
                    continue;
                }
            }
            ParseRecords(reader, static_cast<CSymTab*>(child), childpath, flag);
            continue;
        }

        strcpy(full, pattern);
        strcpy(full + strlen(full), fd.name);
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char splitName[_MAX_FNAME];
        char fname[0x108];
        char ext[0x108];
        _splitpath(full, drive, dir, splitName, ext);
        strcpy(fname, splitName);
        _strupr(fname);
        i32 nleft = static_cast<i32>(strlen(fname));
        i32 i = 0;
        while (i < nleft && fname[i] >= '0' && fname[i] <= '9') {
            i++;
        }
        i32 key = (i >= nleft) ? atoi(fname) : static_cast<i32>(m_nextGeneratedFileKey++);
        u32 extKey = 0;
        char extName[0x10];
        char unpackedTag[0x10];
        if (ext[0] != 0) {
            strcpy(extName, ext + 1);
            _strupr(extName);
            extKey = PackTag(extName);
        }
        UnpackTag(extKey, unpackedTag);
        CSymRec* rec = node->FindOrAddSym(static_cast<i32>(extKey));
        CParseSource* entry = node->Insert(fname, extKey);
        CParseSource* source = 0;
        if (entry == 0) {
            source = node->AddNodeEntry(static_cast<u32>(key), fname, rec, 0);
        } else if (flag != 0) {
            node->AddNodeSubEntry(rec, entry);
            source = node->AddNodeEntry(static_cast<u32>(key), fname, rec, 0);
        }
        if (source != 0) {
            source->m_typeTag = static_cast<i32>(fd.time_write);
            source->m_length = static_cast<u32>(fd.size);
            source->m_reader = new CRezFile(this, full, static_cast<CRezDir*>(reader));
        }
    } while (_findnext(h, &fd) != 0);
    _findclose(h);
    return 1;
}

// @early-stop
RVA(0x0013b850, 0xa8)
i32 CSymParser::Clear(i32 final) {
    static_cast<void>(final);
    i32 r = m_activeNode->Close();
    m_list.Remove(m_activeNode);
    m_list.m_count--;
    delete m_activeNode;
    m_activeNode = 0;
    CRezItmBase* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        p->Close();
        m_list.Remove(p);
        m_list.m_count--;
        delete p;
    }
    if (m_root) {
        delete m_root;
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    m_parseArmed = 0;
    return r;
}

RVA(0x0013b910, 0x58)
u32 CSymParser::PackTag(const char* s) {
    if (!s) {
        return 0;
    }
    DwordBytes r;
    r.m_v = 0;
    u8* rb = r.m_b;
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
    return r.m_v;
}

RVA(0x0013b970, 0x72)
void __stdcall UnpackTag(u32 tag, char* dst) {
    if (!dst) {
        return;
    }
    // Byte-evidenced dword/byte representation seam.
    u8* tb = reinterpret_cast<DwordBytes*>(&tag)->m_b;
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
    for (CRezItmBase* n = m_list.m_head; n != 0; n = n->m_next) {
        if (n->Check() == 0) {
            ok = 0;
        }
    }
    return ok;
}

RVA(0x0013ba70, 0x10)
i32 CSymParser::MakeSeed() {
    time_t t;
    return static_cast<i32>(time(&t));
}

RVA(0x0013ba80, 0x57)
void CSymParser::SetDelims(char* s) {
    if (m_delims != 0) {
        ::operator delete(m_delims);
    }
    m_delims = static_cast<char*>(::operator new(strlen(s) + 1));
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
CParseSource* CSymTab::ResolveQualified(const char* name, i32 fourcc) {
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

RVA(0x0013b900, 0x4)
CSymTab* CSymParser::GetRoot() {
    return m_root;
}

RVA(0x0013bff0, 0x19)
CParseSource* CSymParser::ResolveQualified(const char* name, u32 fourcc) {
    return GetRoot()->ResolveQualified(name, fourcc);
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
    RezFindRec rec;
    if (_stat(name, &rec) != 0) {
        return 0;
    }
    // Language-forced view of packed record storage.

    return (*reinterpret_cast<i32*>((rec.raw + 6)) & 0x4000) == 0x4000;
}

// @early-stop
RVA(0x0013c0c0, 0x14b)
CParseSource* CSymParser::PopParseSlot() {
    void* rec = 0;
    CHashElement* e = m_hash.First();
    if (e != 0) {
        rec = e->m_parseSource;
    }
    if (rec == 0) {
        CSlotNode* node = static_cast<CSlotNode*>(RezAlloc(0xc));
        if (node == 0) {
            return 0;
        }
        CParseSource* arr = new CParseSource[m_parseSlotBlockCount];
        node->m_buffer = arr;
        if (arr == 0) {
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
