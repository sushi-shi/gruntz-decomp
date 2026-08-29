#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/zBitVec.h>

#include <stddef.h>

struct CVariantSlot;
struct CButeValue;
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
        return d ? right : left;
    }

    zPTreeNode* left;
    zPTreeNode* right;
    i32 index;
    char* symbol;
    void* body;
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
        return _count;
    }

    i32 valid() const {
        return !(flags & IDX(SUSPECT));
    }

    void noclean() {
        flags |= IDX(NONE);
    }

    static i32 same(void* a, void* b) {
        return a == b;
    }

    i32 purge() const {
        return flags & IDX(ACTIVE);
    }

    i32 leave() const {
        return flags & IDX(NONE);
    }

    void makevalid() {
        flags &= ~IDX(SUSPECT);
    }

    void invalidate() {
        flags |= IDX(SUSPECT);
    }

    void incc() {
        ++_count;
    }

    void decc() {
        --_count;
    }

    void resetc(size_t value = 0) {
        _count = value;
    }

    void destroy(void* value) {
        dtor(value);
    }

    virtual ~zPtrColl();

protected:
    zPtrColl(cleanup_behaviour cleanup, dtorf_t destructor);

    dtorf_t destructor() const {
        return dtor;
    }

private:
    dtorf_t dtor;
    i16 flags;
    char m_pada[2];
    size_t _count;
};

class zPTree : public zErrHandling, public zPtrColl {
public:
    RVA(0x000212a0, 0x21)
    void clear() {
        cleanup();
        root = NULL;
        preview = false;
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

    zPTreeNode* root;
    zPTreeNode* p;
    zPTreeNode* q;
    i32 sbits;
    i32 preview;
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
