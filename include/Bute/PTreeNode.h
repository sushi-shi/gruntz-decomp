#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/zBitVec.h>

#include <stddef.h>

struct CVariantSlot;
extern CVariantSlot g_rezArchiveErrorSlot;

typedef void(__cdecl* dtorf_t)(void*);
typedef void(__cdecl* stvf_t)(const char*, void*, void*);

GZ_ENUM_CONST_BEGIN(PTreeBitLayout)
    PTREE_BITS_PER_BYTE = 8,
    PTREE_BYTE_BIT_SHIFT = 3,
    PTREE_BYTE_BIT_MASK = 7
GZ_ENUM_CONST_END(PTreeBitLayout)

class zPTreeNode {
    friend class zPTree;

    zPTreeNode*& ptr(i32 d) {
        return d ? m_right : m_left;
    }

    zPTreeNode* m_left;
    zPTreeNode* m_right;
    i32 m_index;
    char* m_symbol;
    void* m_body;
};

class zPtrColl {
public:
    GZ_ENUM_BEGIN(cleanup_behaviour)
        PASSIVE = 0,
        NONE = 1,
        ACTIVE = 2
    GZ_ENUM_END(cleanup_behaviour)

    GZ_ENUM_BEGIN(marker_validity)
        SUSPECT = 4
    GZ_ENUM_END(marker_validity)

    size_t count() const {
        return m_count;
    }

    i32 valid() const {
        return !(m_flags & IDX(SUSPECT));
    }

    void noclean() {
        m_flags |= IDX(NONE);
    }

    static i32 same(void* a, void* b) {
        return a == b;
    }

    i32 purge() const {
        return m_flags & IDX(ACTIVE);
    }

    i32 leave() const {
        return m_flags & IDX(NONE);
    }

    void makevalid() {
        m_flags &= ~IDX(SUSPECT);
    }

    void invalidate() {
        m_flags |= IDX(SUSPECT);
    }

    void incc() {
        ++m_count;
    }

    void decc() {
        --m_count;
    }

    void resetc(size_t value = 0) {
        m_count = value;
    }

    void destroy(void* value) {
        m_dtor(value);
    }

    virtual ~zPtrColl();

protected:
    zPtrColl(cleanup_behaviour cleanup, dtorf_t destructor);

    dtorf_t destructor() const {
        return m_dtor;
    }

private:
    dtorf_t m_dtor;
    i16 m_flags;
    size_t m_count;
};

class zPTree : public zErrHandling, public zPtrColl {
public:
    RVA(0x000212a0, 0x21)
    void clear() {
        cleanup();
        m_root = NULL;
        m_preview = false;
        resetc();
    }

protected:
    zPTree(dtorf_t destructor, cleanup_behaviour cleanup = ACTIVE);

    virtual ~zPTree() OVERRIDE {
        cleanup();
    }

    void* insert(const char* key, void* value);

    void* lookup(const char* key);

    void* add(const char* key, void* value);

    void _trav(stvf_t fn, void* supplementary, zPTreeNode* node);

private:
    void cleanup(zPTreeNode* node = NULL);

    static i32 bit(const char* s, i32 n) {
        return s[n >> PTREE_BYTE_BIT_SHIFT] & (1 << (n & PTREE_BYTE_BIT_MASK));
    }

    static i32 diffpos(const char* a, const char* b);

    zPTreeNode* m_root;
    zPTreeNode* m_p;
    zPTreeNode* m_q;
    i32 m_sbits;
    i32 m_preview;
};

template<class T> class zSymTab : public zPTree {
public:
    zSymTab(cleanup_behaviour cleanup = ACTIVE)
        : zPTree(
              // PROVEN: original zSymTab erases its typed teardown callback at this ABI seam.
              reinterpret_cast<dtorf_t>(dtf),
              cleanup
          ) {}

    T* insert(const char* key, T* value) {
        return static_cast<T*>(zPTree::insert(key, value));
    }

    T* lookup(const char* key) {
        return static_cast<T*>(zPTree::lookup(key));
    }

    T* add(const char* key, T* value) {
        return static_cast<T*>(zPTree::add(key, value));
    }

    void traverse(
        void(__cdecl* fn)(const char* key, T* value, void* supplementary),
        void* supplementary = NULL
    ) {
        zPTree::_trav(
            // PROVEN: original zSymTab erases its typed traversal callback at this ABI seam.
            reinterpret_cast<stvf_t>(fn),
            supplementary,
            NULL
        );
    }

private:
    static void dtf(T* p) {
        p->T::~T();
    }

    zSymTab(const zSymTab<T>&);
    zSymTab<T>& operator=(const zSymTab<T>&);
};

#endif // SRC_BUTE_PTREENODE_H
