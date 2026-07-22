#define SBI_DTOR_CHAIN        // enable the inline base-dtor bodies down the chain
#define SBI_OWN_RECTONLY_DTOR // this TU supplies the out-of-line ~CSBI_RectOnly (0x100700)
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_Image.h> // canonical CSBI_RectOnly chain + CStatusBarItem device

// Stamp ??_7CSBI_RectOnly, run DtorRect, then MSVC folds the base dtor (stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobject supplies the
// /GX frame and the 0/-1 trylevel stamps.
//
// DtorRect() is the class's member teardown @0xe8760 (a 1-byte `ret` - CSBI_RectOnly adds
// no fields); retail routes this call through the ILT jmp-thunk 0x1bd1 (-> 0xe8760). The
// body is an orphan/library-carveout empty stub (unreconstructed), so bind the alias to
// the THUNK the call literally targets; reloc_fidelity thunk-resolves both sides to
// 0xe8760 -> CORRECT (a future pass may home the empty 0xe8760 body proper).
DATA_SYMBOL(0x00001bd1, 0x0, ?DtorRect@CSBI_RectOnly@@QAEXXZ)
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

// -------------------------------------------------------------------------
// 0x100780 == the standalone out-of-line CStatusBarItem::~CStatusBarItem() COMDAT.
// The /GX funclet above ODR-uses ??1CStatusBarItem (the base-subobject unwind leg),
// so cl emits its inline body (`mov [ecx],??_7CStatusBarItem; jmp DtorStatus`) as an
// out-of-line COMDAT - byte-identical to retail 0x100780. Bind that emitted symbol to
// its RVA (like the ??_G scalar-dtor above); this replaces the former fake
// CStatusBaseSub100780 placeholder view (whose ??_7/Base1d6b reloc-masked the real
// ??_7CStatusBarItem/DtorStatus by shape) and binds BOTH the funclet reloc @0x100700
// and this body's DtorStatus @0x10bfa0.
RVA_COMPGEN(0x00100780, 0xb, ??1CStatusBarItem@@UAE@XZ)
