#include <rva.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/BlowfishApi.h>
#include <EmptyString.h>
#include <strstrea.h>

#include <float.h>
#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>

static i16 g_tokenLen;

static const float s_floatErr = FLT_MIN;
static const double s_doubleErr = DBL_MIN;

static const char s_strFloatSuffix[] = "f";
static const char s_strOpen[] = "(";
static const char s_strClose[] = ")";
static const char s_strComma[] = ", ";
static const char s_strLt[] = "<";
static const char s_strGt[] = ">";
static const char s_strLBrack[] = "[";
static const char s_strRBrack[] = "]";

// @identity-TODO ?_GzPTree - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (50 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x000212e0, 0x1e, ??_GzPTree@@UAEPAXI@Z)
// @identity-TODO ?1zPTree - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (50 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00021310, 0x70, ??1zPTree@@UAE@XZ)

// @identity-TODO ?1CButeMgr - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (50 fns) came from the static library. It belongs to another compiland.
RVA(0x000213c0, 0x14c)
CButeMgr::~CButeMgr() {}

// @identity-TODO ?1CBSecStream - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (50 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00021570, 0x70, ??1CBSecStream@@UAE@XZ)

// @identity-TODO ?_EzPTree - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (50 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00021600, 0x8, ??_EzPTree@@W7AEPAXI@Z)
