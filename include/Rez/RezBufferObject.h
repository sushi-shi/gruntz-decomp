#pragma once
#include <Mfc.h>
#include <MfcWin.h>
#include <afxtempl.h>
#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h>

#include <stddef.h>

struct RezElem40 {
    CRect m_startRect;
    CRect m_endRect;
    i32 m_reserved20;
    float m_scale;
};

typedef CArray<RezElem40, const RezElem40&> CRezBufferObject;
