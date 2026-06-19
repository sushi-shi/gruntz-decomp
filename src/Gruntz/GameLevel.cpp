// GameLevel.cpp - CGameLevel::LoadWwd, the WWD level-load orchestrator.
//
// Functions in this TU:
//   CGameLevel::LoadWwd  @ RVA 0x15d280 (633 B, __thiscall ret 0x4, vtable 0x38)
//
// LoadWwd is the level-load driver. Faithful reconstruction (carcass driven toward
// byte-exact; see header for the member layout). Flow, straight off the bytes:
//
//   1. Reset()                              (this->vtable[+0x44])
//   2. if (hdr->wwdSignature > 0x5F4) return 0;
//   3. m_header = *hdr;                      (rep movs 0x17d dwords = 1524 B)
//   4. if (hdr->flags & COMPRESS) {          (test [hdr+8],0x2)
//        char* block = (char*)operator new(hdr->mainBlockLength
//                                           + hdr->wwdSignature + 0x40);
//        if (!block) return 0;
//        block = WwdFile_InflateMainBlock(hdr, block, size);
//        if (!block) { operator delete(...); return 0; }   // ehAlloc tracked
//      } else block = (char*)hdr;            (uncompressed: read in place)
//   5. strcpy(m_levelName, hdr->levelName);  (inline strlen + rep movs)
//      m_flags = hdr->flags; m_checksum = hdr->checksum;
//   6. for (i = 0; i < hdr->numPlanes; ++i)  (plane cursor stride 0xA0)
//        if (!ReadPlane(cursor, block, &m_planeCtx)) goto fail;
//   7. if (hdr->tileDescriptionsOffset) {    (image-set descriptors)
//        for (j = 0; j < count; ++j) {
//          CImageSet* s = ReadImageSet(cursor); if (!s) goto fail;
//          m_imageSets.SetAtGrow(j, s); cursor += s->GetStride();
//        }
//      }
//   8. recompute the scaled start coords on the main plane + every plane;
//      free the tracked inflate buffer; return 1.
//
// CPlane / CImageSet / the per-plane reader / ReadImageSet / RecomputePlaneCoords /
// InflateMainBlock / operator new/delete / SetAtGrow are reloc-masked calls.
#include "GameLevel.h"
#include "../rva.h"

#include <string.h>  // strcpy

RVA(0x15d280, 0x279)
int CGameLevel::LoadWwd(WwdHeader* hdr)
{
    Reset();                                  // vtable +0x44

    if (hdr->wwdSignature > 0x5f4)            // signature must be <= 1524
        return 0;

    // Copy the 1524-byte header into the level object (rep movs 0x17d dwords).
    m_header = *hdr;

    char* block;
    char* ehAlloc = 0;                        // inflate buffer tracked by the EH state

    if (hdr->flags & 0x2)                     // COMPRESS: inflate the main block
    {
        unsigned int allocSize =
            hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        char* buf = (char*)operator new(allocSize);
        if (buf == 0)
            return 0;

        block = (char*)WwdFile_InflateMainBlock((WwdHeader*)hdr, (Bytef*)buf,
                                                allocSize - 0x20);
        if (block == 0)
        {
            operator delete(buf);
            return 0;
        }
        ehAlloc = buf;
    }
    else
    {
        block = (char*)hdr;                   // uncompressed: planes follow in place
    }

    strcpy(m_levelName, hdr->levelName);      // inline strlen + rep movs
    m_flags = hdr->flags;
    m_checksum = hdr->checksum;

    // --- plane loop ---------------------------------------------------------
    char* cursor = block + hdr->planesOffset;
    unsigned int i = 0;
    if (hdr->numPlanes != 0)
    {
        do
        {
            if (ReadPlane(cursor, block, &m_planeCtx) == 0)
                goto fail;
            ++i;
            cursor += 0xa0;                   // WwdPlaneHeader stride
        } while (i < hdr->numPlanes);
    }

    // --- image-set descriptors ---------------------------------------------
    if (hdr->tileDescriptionsOffset != 0)
    {
        char* rec = block + hdr->tileDescriptionsOffset;
        // (the target re-tests the cursor against the record header before the
        // loop; the descriptor count lives in the descriptor block header.)
        unsigned int count = *(unsigned int*)(rec + 0x8);
        unsigned int j = 0;
        while (j < count)
        {
            CImageSet* set = ReadImageSet(rec + 0x20);
            if (set == 0)
                goto fail;
            ++j;
            rec += set->GetStride();          // vtable +0x24 stride advance
            m_imageSets.SetAtGrow((int)(j - 1), set);
        }
    }

    // --- scaled start coords on the main plane + every plane ---------------
    // For each plane the WWD start coords are placed (and, unless the plane has
    // the origin-fixed flag bit0, multiplied by the plane's parallax factors)
    // into m_scaledX/m_scaledY; then the per-plane coord recompute runs.
    {
        int startX = hdr->startX;
        int startY = hdr->startY;
        CPlane* mp = m_mainPlane;
        if (mp->m_flags & 1)
        {
            mp->m_scaledX = (float)startX;
            mp->m_scaledY = (float)startY;
        }
        else
        {
            mp->m_scaledX = (float)startX * mp->m_scaleX;
            mp->m_scaledY = (float)startY * mp->m_scaleY;
        }
        RecomputePlaneCoords(mp);

        // Re-derive the start coords from the main plane's origin for the rest.
        int ox = m_mainPlane->m_originX;
        int oy = m_mainPlane->m_originY;
        int i2 = 0;
        while (i2 < m_planes.m_size)          // m_planes.m_size == the plane count
        {
            if (i2 != m_mainIndex)
            {
                CPlane* p = (CPlane*)((void**)m_planes.m_data)[i2];
                if (p->m_flags & 1)
                {
                    p->m_scaledX = (float)ox;
                    p->m_scaledY = (float)oy;
                }
                else
                {
                    p->m_scaledX = (float)ox * p->m_scaleX;
                    p->m_scaledY = (float)oy * p->m_scaleY;
                }
                RecomputePlaneCoords(p);
            }
            ++i2;
        }
    }

    if (ehAlloc != 0)
        operator delete(ehAlloc);
    return 1;

fail:
    if (ehAlloc != 0)
        operator delete(ehAlloc);
    return 0;
}
