#include <rva.h>

#include <Lith/BaseHash.h>

#include <stddef.h>

RVA(0x001848b0, 0x47)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184900, 0x43)
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

RVA(0x00184950, 0x10)
CBaseHash::CBaseHash() {
    m_nNumBins = 0;
    m_pBinAry = NULL;
}

RVA(0x00184960, 0x70)
CBaseHash::CBaseHash(u32 numBins) {
    m_nNumBins = numBins;
    m_pBinAry = new CHashBin[m_nNumBins];
}

RVA_COMPGEN(0x001849d0, 0x50, ??_ECHashBin@CBaseHash@@QAEPAXI@Z)
RVA_COMPGEN(0x00184a20, 0xb, ??0CHashBin@CBaseHash@@QAE@XZ)
RVA_COMPGEN(0x00184a30, 0x1, ??1CHashBin@CBaseHash@@QAE@XZ)

RVA(0x00184a40, 0x27)
CBaseHash::~CBaseHash() {
    if (m_pBinAry != NULL) {
        delete[] m_pBinAry;
    }
}

RVA(0x00184a70, 0x34)
void CBaseHash::Insert(CBaseHashItem* item) {
    item->m_pParentHash = this;
    u32 curBin = item->HashFunc();
    item->m_nCurBin = curBin;
    m_pBinAry[curBin].m_lstItems.InsertFirst(item);
}

RVA(0x00184ab0, 0x25)
void CBaseHash::Delete(CBaseHashItem* item) {
    m_pBinAry[item->m_nCurBin].m_lstItems.Delete(item);
}

RVA(0x00184ae0, 0x24)
CBaseHashItem* CBaseHash::GetFirst() {
    u32 bin = 0;
    CBaseHashItem* item;
    do {
        item = static_cast<CBaseHashItem*>(m_pBinAry[bin].m_lstItems.GetFirst());
        bin++;
    } while (item == NULL && bin < m_nNumBins);
    return item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184b10, 0x29)
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

RVA(0x00184b40, 0x1d)
CBaseHashItem* CBaseHash::GetFirstInBin(u32 bin) {
    return static_cast<CBaseHashItem*>(m_pBinAry[bin].m_lstItems.GetFirst());
}
