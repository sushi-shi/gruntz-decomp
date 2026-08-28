#ifndef LITH_BASEHASH_H
#define LITH_BASEHASH_H

#include <Ints.h>
#include <Lith/BaseList.h>

class CBaseHashItem;

class CBaseHash {
public:
    CBaseHash();
    CBaseHash(u32 numBins);
    ~CBaseHash();

    void Insert(CBaseHashItem* item);
    void Delete(CBaseHashItem* item);
    CBaseHashItem* GetFirst();
    CBaseHashItem* GetLast();

protected:
    CBaseHashItem* GetFirstInBin(u32 bin);

    u32 GetNumBins() {
        return m_nNumBins;
    }

private:
    friend class CBaseHashItem;

    class CHashBin : CBaseListItem {
    public:
        CLTBaseList m_lstItems;
    };

    u32 m_nNumBins;
    CHashBin* m_pBinAry;
};

class CBaseHashItem : public CBaseListItem {
public:
    CBaseHashItem* Next();
    CBaseHashItem* Prev();

    CBaseHashItem* NextInBin() {
        return static_cast<CBaseHashItem*>(CBaseListItem::Next());
    }

    CBaseHashItem* PrevInBin() {
        return static_cast<CBaseHashItem*>(CBaseListItem::Prev());
    }

protected:
    virtual u32 HashFunc() = 0;

    CBaseHash* GetParentHash() {
        return m_pParentHash;
    }

private:
    friend class CBaseHash;

    CBaseHash* m_pParentHash;
    u32 m_nCurBin;
};

#endif // LITH_BASEHASH_H
