#include <rva.h>
// CSBI_Image.cpp - engine-label stubs for CSBI_Image.
//
// NOTE: 0x10a340 was mislabeled `~CSBI_Image` by the rtti-vptr heuristic; the real
// scalar destructor is reconstructed at 0x100870 (src/Gruntz/SBI_ImageEh.cpp).
// Relabeled `Method_10a340` to free the ??1 name for the real dtor.
//
// STRUCTURAL DECODE (for the final-sweep leaf-first redo -- NOT the dtor, and NOT
// a serialize walk): 0x10a340 is the /GX-framed "build the TABZ_DIALOG" factory on
// the CSBI_RectOnly host (this=ebx; ebp pinned 0). It gates on m_550, copies the
// m_c sub-object's RECT (m_c->m_24+0x10) via CopyRect (import 0x6c44bc), derives
// (cx,cy) = rect centre, then branches on m_554:
//   * m_554 != 0  -> a 3-item confirm dialog: AREYOUSURE (t3 CSBI_Image 0x34, cmd
//     0x321), YES (menuItem CSBI_MenuItem 0x3c, cmd 0x327, store m_1fc), NO
//     (menuItem, cmd 0x328, store m_200).
//   * m_554 == 0  -> the main tabz dialog: DIALOG (t3, cmd 0x321), then a decision
//     tree on g_mgrSettings->m_68->m_288==1 (mission complete) and ->m_134==1
//     (test mode) and the 4-player m_17c active-count (>=2 => multiplayer):
//     MISSIONSTATUS/REASON (t4 CSBI_ImageSet 0x3c, cmd 0x322/0x326), PLAYNEXTLEVEL
//     / REPLAYLEVEL (menuItem, cmd 0x324, store m_1f4), QUITTOMAINMENU/OBSERVE
//     (cmd from 0x614c08/0x614bdc), STATZ (cmd 0x325, store m_1f8/m_578=1).
// Each item: operator_new(0x34|0x3c) -> out-of-line base ctor (CStatusBarItem
// @0x22c0 for t3/some menuItems, CSBI_RectOnly @0x1e88 for t4/others) -> manual
// vtable stamp (t3=?g_vtbl_t3@0x5eac0c / menuItem=?g_vtbl_menuItem@0x5eab4c /
// t4=?g_vtbl_t4@0x5eac4c) + m_8=3/2/4 + zero m_30/m_34/m_38 -> the 11-arg
// vtable-slot-11 setup call [vptr+0x2c](this, m_c, cmd, 6, x0,y0,x1,y1, key,
// a10, 0). On setup==0: scalar-delete [vptr+0](1) + return 0. On success:
// this->m_d4.AddTail(item) (CObList::AddTail @0x1b4991) + the per-item field store.
// Returns 1. EH: per-item trylevel stamps [esp+0x48]=0..0xb / [esp+0x6c]=-1.
// WALLS for the redo: (1) EH-state numbering + the /GX __ehfuncinfo; (2) the
// interwoven shared setup/fail tails (0x10ac63/0x10adc6/0x10ad3a) do not map to
// nested if/else -> a plain reconstruction diverges structurally; (3) the 3019 B
// regalloc cascade. Branch A + the DIALOG head are fully decoded above; the rest
// of branch B's geometry needs a dedicated pass. Deferred (too big to converge in
// one pass without diverging).

class CSBI_Image {
public:
    void Method_10a340();
};

// @confidence: low
// @source: rtti-vptr
// @stub
RVA(0x0010a340, 0xbcb)
void CSBI_Image::Method_10a340() {}
