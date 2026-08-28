#ifndef REZ_REZHASH_H
#define REZ_REZHASH_H

#include <rva.h>

#include <Ints.h>
#include <Lith/BaseHash.h>

class CRezTyp;
class CRezDir;
struct CRezItm;

class CRezItmHashTableByName;

class CRezItmHashByName : public CBaseHashItem {
public:
    CRezItmHashByName() : CBaseHashItem() {
        m_pRezItm = NULL;
    }

    CRezItmHashByName(CRezItm* rezItm) : CBaseHashItem() {
        m_pRezItm = rezItm;
    }

    void SetRezItm(CRezItm* rezItm) {
        m_pRezItm = rezItm;
    }

    CRezItm* GetRezItm() {
        return m_pRezItm;
    }

    CRezItmHashByName* Next() {
        return static_cast<CRezItmHashByName*>(CBaseHashItem::Next());
    }

    CRezItmHashByName* Prev() {
        return static_cast<CRezItmHashByName*>(CBaseHashItem::Prev());
    }

    CRezItmHashByName* NextInBin() {
        return static_cast<CRezItmHashByName*>(CBaseHashItem::NextInBin());
    }

    CRezItmHashByName* PrevInBin() {
        return static_cast<CRezItmHashByName*>(CBaseHashItem::PrevInBin());
    }

protected:
    virtual u32 HashFunc() OVERRIDE;

    CRezItmHashTableByName* GetParentHash();

private:
    friend class CRezItmHashTableByName;

    CRezItm* m_pRezItm;
};

class CRezItmHashTableByName : public CBaseHash {
public:
    CRezItmHashTableByName(u32 numBins) : CBaseHash(numBins) {}
    CRezItmHashTableByName() : CBaseHash(1) {}

    CRezItm* Find(const char* name, i32 ignoreCase = 1);

    void Insert(CRezItmHashByName* item) {
        CBaseHash::Insert(item);
    }

    void Delete(CRezItmHashByName* item) {
        CBaseHash::Delete(item);
    }

    CRezItmHashByName* GetFirst() {
        return static_cast<CRezItmHashByName*>(CBaseHash::GetFirst());
    }

    CRezItmHashByName* GetLast() {
        return static_cast<CRezItmHashByName*>(CBaseHash::GetLast());
    }

protected:
    friend class CRezItmHashByName;

    CRezItmHashByName* GetFirstInBin(u32 bin) {
        return static_cast<CRezItmHashByName*>(CBaseHash::GetFirstInBin(bin));
    }

    u32 HashFunc(const char* text);
};

inline CRezItmHashTableByName* CRezItmHashByName::GetParentHash() {
    return static_cast<CRezItmHashTableByName*>(CBaseHashItem::GetParentHash());
}

class CRezItmHashTableByID;

class CRezItmHashByID : public CBaseHashItem {
public:
    CRezItmHashByID() : CBaseHashItem() {
        m_pRezItm = NULL;
    }

    CRezItmHashByID(CRezItm* rezItm) : CBaseHashItem() {
        m_pRezItm = rezItm;
    }

    void SetRezItm(CRezItm* rezItm) {
        m_pRezItm = rezItm;
    }

    CRezItm* GetRezItm() {
        return m_pRezItm;
    }

    CRezItmHashByID* Next() {
        return static_cast<CRezItmHashByID*>(CBaseHashItem::Next());
    }

    CRezItmHashByID* Prev() {
        return static_cast<CRezItmHashByID*>(CBaseHashItem::Prev());
    }

    CRezItmHashByID* NextInBin() {
        return static_cast<CRezItmHashByID*>(CBaseHashItem::NextInBin());
    }

    CRezItmHashByID* PrevInBin() {
        return static_cast<CRezItmHashByID*>(CBaseHashItem::PrevInBin());
    }

protected:
    virtual u32 HashFunc() OVERRIDE;

    CRezItmHashTableByID* GetParentHash();

private:
    friend class CRezItmHashTableByID;

    CRezItm* m_pRezItm;
};

class CRezItmHashTableByID : public CBaseHash {
public:
    CRezItmHashTableByID(u32 numBins) : CBaseHash(numBins) {}
    CRezItmHashTableByID() : CBaseHash() {}

    void Insert(CRezItmHashByID* item) {
        CBaseHash::Insert(item);
    }

    void Delete(CRezItmHashByID* item) {
        CBaseHash::Delete(item);
    }

    CRezItmHashByID* GetFirst() {
        return static_cast<CRezItmHashByID*>(CBaseHash::GetFirst());
    }

    CRezItmHashByID* GetLast() {
        return static_cast<CRezItmHashByID*>(CBaseHash::GetLast());
    }

protected:
    friend class CRezItmHashByID;

    CRezItmHashByID* GetFirstInBin(u32 bin) {
        return static_cast<CRezItmHashByID*>(CBaseHash::GetFirstInBin(bin));
    }
};

inline CRezItmHashTableByID* CRezItmHashByID::GetParentHash() {
    return static_cast<CRezItmHashTableByID*>(CBaseHashItem::GetParentHash());
}

class CRezTypeHashTable;

class CRezTypeHash : public CBaseHashItem {
public:
    CRezTypeHash() : CBaseHashItem() {
        m_pRezTyp = NULL;
    }

    CRezTypeHash(CRezTyp* rezTyp) : CBaseHashItem() {
        m_pRezTyp = rezTyp;
    }

    void SetRezTyp(CRezTyp* rezTyp) {
        m_pRezTyp = rezTyp;
    }

    CRezTyp* GetRezTyp() {
        return m_pRezTyp;
    }

    CRezTypeHash* Next() {
        return static_cast<CRezTypeHash*>(CBaseHashItem::Next());
    }

    CRezTypeHash* Prev() {
        return static_cast<CRezTypeHash*>(CBaseHashItem::Prev());
    }

    CRezTypeHash* NextInBin() {
        return static_cast<CRezTypeHash*>(CBaseHashItem::NextInBin());
    }

    CRezTypeHash* PrevInBin() {
        return static_cast<CRezTypeHash*>(CBaseHashItem::PrevInBin());
    }

protected:
    virtual u32 HashFunc() OVERRIDE;

    CRezTypeHashTable* GetParentHash();

private:
    friend class CRezTypeHashTable;

    CRezTyp* m_pRezTyp;
};

class CRezTypeHashTable : public CBaseHash {
public:
    CRezTypeHashTable(u32 numBins) : CBaseHash(numBins) {}

    CRezTyp* Find(u32 type);

    void Insert(CRezTypeHash* item) {
        CBaseHash::Insert(item);
    }

    void Delete(CRezTypeHash* item) {
        CBaseHash::Delete(item);
    }

    CRezTypeHash* GetFirst() {
        return static_cast<CRezTypeHash*>(CBaseHash::GetFirst());
    }

    CRezTypeHash* GetLast() {
        return static_cast<CRezTypeHash*>(CBaseHash::GetLast());
    }

protected:
    friend class CRezTypeHash;

    CRezTypeHash* GetFirstInBin(u32 bin) {
        return static_cast<CRezTypeHash*>(CBaseHash::GetFirstInBin(bin));
    }

    u32 HashFunc(u32 type);
};

inline CRezTypeHashTable* CRezTypeHash::GetParentHash() {
    return static_cast<CRezTypeHashTable*>(CBaseHashItem::GetParentHash());
}

class CRezDirHashTable;

class CRezDirHash : public CBaseHashItem {
public:
    CRezDirHash() : CBaseHashItem() {
        m_pRezDir = NULL;
    }

    CRezDirHash(CRezDir* rezDir) : CBaseHashItem() {
        m_pRezDir = rezDir;
    }

    void SetRezDir(CRezDir* rezDir) {
        m_pRezDir = rezDir;
    }

    CRezDir* GetRezDir() {
        return m_pRezDir;
    }

    CRezDirHash* Next() {
        return static_cast<CRezDirHash*>(CBaseHashItem::Next());
    }

    CRezDirHash* Prev() {
        return static_cast<CRezDirHash*>(CBaseHashItem::Prev());
    }

    CRezDirHash* NextInBin() {
        return static_cast<CRezDirHash*>(CBaseHashItem::NextInBin());
    }

    CRezDirHash* PrevInBin() {
        return static_cast<CRezDirHash*>(CBaseHashItem::PrevInBin());
    }

protected:
    virtual u32 HashFunc() OVERRIDE;

    CRezDirHashTable* GetParentHash();

private:
    friend class CRezDirHashTable;

    CRezDir* m_pRezDir;
};

class CRezDirHashTable : public CBaseHash {
public:
    CRezDirHashTable(u32 numBins) : CBaseHash(numBins) {}

    CRezDir* Find(const char* name, i32 ignoreCase = 1);

    void Insert(CRezDirHash* item) {
        CBaseHash::Insert(item);
    }

    void Delete(CRezDirHash* item) {
        CBaseHash::Delete(item);
    }

    CRezDirHash* GetFirst() {
        return static_cast<CRezDirHash*>(CBaseHash::GetFirst());
    }

    CRezDirHash* GetLast() {
        return static_cast<CRezDirHash*>(CBaseHash::GetLast());
    }

protected:
    friend class CRezDirHash;

    CRezDirHash* GetFirstInBin(u32 bin) {
        return static_cast<CRezDirHash*>(CBaseHash::GetFirstInBin(bin));
    }

    u32 HashFunc(const char* text);
};

inline CRezDirHashTable* CRezDirHash::GetParentHash() {
    return static_cast<CRezDirHashTable*>(CBaseHashItem::GetParentHash());
}

#endif // REZ_REZHASH_H
