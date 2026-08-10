#include <rva.h>

#include <Gruntz/ActReg.h>

// The only zDArray specialization retail instantiates is T = CActHandler, a
// pointer-to-member-function, so the per-element construct/destruct walk that
// these two bodies head is empty and /O2 drops it. What survives in retail is
// the cursor itself: a 4-byte memory-resident pointer initialised from the
// element buffer and never read again.
//
//   ctor  mov edx,[esi+0x1c]        ; m_alloc (== m_base, set by _zdvec's ctor)
//         mov [esp+0xc],edx         ; spilled into the dead `hi` param slot
//   dtor  push ecx / pop ecx        ; a 4-byte local frame - which is why the
//         mov eax,[ecx+0x10]        ; dtor *calls* ~_zvec instead of jmp-ing to
//         mov [esp],eax             ; it, as a body-less dtor would
//
// @early-stop
// The cursor store is what is missing. MSVC 5.0 /O2 dead-store-eliminates every
// spelling that keeps the source honest - plain local, address-taken local, the
// AsElem() union pun, a directly declared AddrWord union, a one-element array, a
// 4-byte memcpy, and a write to the `hi` parameter were each measured and each
// vanished. The construct that homes the cursor to memory before the walk is
// deleted is unrecovered; it needs the cursor's address to escape into an inline
// (a `T*&` element-walk helper is the shape that would do it), and no such
// helper is modelled yet. Until it is, the ctor loses its two trailing
// instructions and the dtor tail-jumps. The previous source forced both with
// `char* volatile`, which is a codegen steer rather than a model, and is removed.
template<> RVA(0x00008710, 0x2b)
zDArray<CActHandler>::zDArray(i32 lo, i32 hi)
    : _zdvec(sizeof(CActHandler), lo, hi, ZVecNoScratch()) {}

// @early-stop
// Same missing cursor store as the constructor above; see the dossier there.
template<> RVA(0x00008750, 0x15)
zDArray<CActHandler>::~zDArray() {}

RVA_COMPGEN(0x00008780, 0x1e, ??_G?$zDArray@P8CUserLogic@@AEHXZ@@UAEPAXI@Z)
