#include <StdAfx.h>

#include <Ints.h>

#include <Lith/BaseHash.h>

#include <stddef.h>

CBaseHashItem* CBaseHashItem::Next() {
    CBaseHashItem* item = static_cast<CBaseHashItem*>(CBaseListItem::Next());
    if (item == NULL) {
        u32 bin = m_nCurBin + 1;
        while (bin < m_pParentHash->m_nNumBins) {
            item = static_cast<CBaseHashItem*>(m_pParentHash->m_pBinAry[bin].m_lstItems.GetFirst());
            if (item != NULL) {
                break;
            }
            bin++;
        }
    }
    return item;
}

CBaseHashItem* CBaseHashItem::Prev() {
    CBaseHashItem* item = static_cast<CBaseHashItem*>(CBaseListItem::Prev());
    if (item == NULL) {
        u32 bin = m_nCurBin;
        while (bin > 0) {
            bin--;
            item = static_cast<CBaseHashItem*>(m_pParentHash->m_pBinAry[bin].m_lstItems.GetLast());
            if (item != NULL) {
                break;
            }
        }
    }
    return item;
}

CBaseHash::CBaseHash() {
    m_nNumBins = 0;
    m_pBinAry = NULL;
}

CBaseHash::CBaseHash(u32 numBins) {
    m_nNumBins = numBins;
    m_pBinAry = new CHashBin[m_nNumBins];
}

CBaseHash::~CBaseHash() {
    if (m_pBinAry != NULL) {
        delete[] m_pBinAry;
    }
}

void CBaseHash::Insert(CBaseHashItem* item) {
    item->m_pParentHash = this;
    u32 curBin = item->HashFunc();
    item->m_nCurBin = curBin;
    m_pBinAry[curBin].m_lstItems.InsertFirst(item);
}

void CBaseHash::Delete(CBaseHashItem* item) {
    m_pBinAry[item->m_nCurBin].m_lstItems.Delete(item);
}

CBaseHashItem* CBaseHash::GetFirst() {
    u32 bin = 0;
    CBaseHashItem* item;
    do {
        item = static_cast<CBaseHashItem*>(m_pBinAry[bin].m_lstItems.GetFirst());
        bin++;
    } while (item == NULL && bin < m_nNumBins);
    return item;
}

CBaseHashItem* CBaseHash::GetLast() {
    u32 bin = m_nNumBins - 1;
    CBaseHashItem* item;
    do {
        item = static_cast<CBaseHashItem*>(m_pBinAry[bin].m_lstItems.GetLast());
        if (bin > 0) {
            bin--;
        } else {
            break;
        }
    } while (item == NULL);
    return item;
}

CBaseHashItem* CBaseHash::GetFirstInBin(u32 bin) {
    return static_cast<CBaseHashItem*>(m_pBinAry[bin].m_lstItems.GetFirst());
}
