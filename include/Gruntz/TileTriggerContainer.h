// TileTriggerContainer.h - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 3-CPtrList container: a base sub-object at +0x00 plus three CPtrList members
// at +0x1c / +0x38 / +0x54 (their m_pNodeHead at +0x20 / +0x3c / +0x58).  The
// lists hold heap command objects; the accessors here move/find/remove those
// objects across the three lists.  The list elements are sibling command objects
// (the CTileTriggerLogic class) destroyed inline (vtable 0x5eaea4 stamp + RezFree)
// before the node is unlinked.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILETRIGGERCONTAINER_H
#define SRC_GRUNTZ_TILETRIGGERCONTAINER_H

#include <Mfc.h> // MFC CPtrList (the m_base/m_list1-3 sub-object list methods)
#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <Gruntz/TileActionEvent.h>   // CTileActionEvent - the 0x28 m_list3 element (was TtcMark)
#include <Gruntz/TileTriggerWiring.h> // CTrigParam / CTrigSourceRecord (AddLogic marshaling blocks)
#include <rva.h>                      // SIZE_UNKNOWN class-metadata macros used below

// The owning container; back-stamped into the list elements (m_14 / m_20 / m_24).
class CTileTriggerContainer;
class CTileTriggerLogic;       // the per-id logic leaf AddLogic news (def in TileTriggerLogic.h)
class CGiantRockLogic;         // the 0xc8 m_list1 rock element (def in TileTriggerLogic.h)
class CTileTriggerSwitchLogic; // the 0x8c m_base element family (def in TileTriggerSwitchLogic.h)

// The Rez heap alloc/free (RVA 0x1b9b46 _RezAlloc / 0x1b9b82 _RezFree);
// reloc-masked rel32 callees.
extern "C" void* RezAlloc(u32 n);
extern "C" void RezFree(void* p);

// The running game clock (DAT_00645588); reloc-masked DIR32 datum.
extern "C" u32 g_frameTime;

// A singly-walked CPtrList node: next@+0x00, data@+0x08 (MFC CList layout, the
// +0x04 prev slot unused by these walkers).
struct TtcNode {
    TtcNode* m_next; // +0x00
    char _pad04[4];  // +0x04 (prev)
    // +0x08  the CPtrList node payload: a genuine heterogeneous CObject* slot - the
    // four lists store DIFFERENT element types (TtcElem / TtcMark / TtcTrigElem /
    // plain i32 records), downcast per walker. Authentic void* (MFC container payload).
    void* m_data;
};
SIZE_UNKNOWN(TtcNode);

// The three list sub-objects ARE the real MFC ::CPtrList (0x1c bytes; its methods are the
// FID-labeled NAFXCW CPtrList members RemoveAt @0x1b4ac7 / AddTail @0x1b4991 / RemoveAll
// @0x1b48a6 / ~CPtrList @0x1b48c6, and its vtable is the library ??_7CObList @0x1ed4b4).
// The former local TtcObList re-declaration (whose emitted ??_7 aliased that bound library
// vtable - a RELOC_VTBL) is DISSOLVED: the walkers reach the head through the INLINE
// CPtrList::GetHeadPosition() (afxcoll.inl: `return (POSITION)m_pNodeHead;` - the identical
// single load) and the count through the inline GetCount().
typedef ::CPtrList TtcObList;
// Typed intrusive-node access: a POSITION IS the CPtrList::CNode* the walkers step through
// (MFC's CNode is protected, so the head is retrieved as a POSITION and typed here).
inline TtcNode* TtcHead(const ::CPtrList& l) {
    return (TtcNode*)l.GetHeadPosition();
}

// The former local element views are DISSOLVED onto the real classes:
//   TtcMark     -> CTileActionEvent   (<Gruntz/TileActionEvent.h>; 0x28 bytes; the
//                  "TtcMarkCtor @0x1e1a" is ??0CTileActionEvent @0x112d80)
//   TtcBaseElem -> CGiantRockLogic    (<Gruntz/TileTriggerLogic.h>; 0xc8 bytes; the
//                  "TtcBaseElemCtor @0x2c3e" is ??0CGiantRockLogic @0x112210 -
//                  SAME ILT thunk; every field matched: type tag 0x16 == factory id
//                  22, m_matrix @+0x9c, m_c0/m_c4, init gate m_1c, owner m_20)
//   TtcKeyedElem-> CTileActionEvent   (the m_playerFlags[4] @+0x18..+0x24 record
//                  FindByField0C returns)
//   TtcTrigElem -> the two real element families. The register thunks resolve to
//                  THREE different targets (retail ILT jmps): 0x277f -> 0x113860 =
//                  CTileTriggerSwitchLogic::ValidateByType, 0x1abe -> 0x113a90 =
//                  CTileTriggerLogic::ValidateByType, 0x1d39 -> 0x113d40 =
//                  CGiantRockLogic::ApplyByType. All __thiscall on the ELEMENT
//                  (retail: `mov ecx,edi` before each call) - the old "all three
//                  are Gate113860, drop the receiver" comment was wrong.

// The two tag-dispatched serialize-and-apply helpers of 117280.  __stdcall free
// functions: stream the element's tag, then dispatch its family's serializer.
// ApplyA serves the m_base 0x8c switch-logic family (tags 1..8); ApplyB serves
// the m_list1/m_list2 CTileTriggerLogic family (tags 0x15..0x1a).
i32 __stdcall
SerializeApplyA(CSerialArchive* s, i32 a2, i32 a3, i32 a4, CTileTriggerSwitchLogic* o); // 0x117630
i32 __stdcall
SerializeApplyB(CSerialArchive* s, i32 a2, i32 a3, i32 a4, CTileTriggerLogic* o); // 0x117710

class CTileTriggerContainer {
public:
    // Inline ctor: the four CPtrList members construct (each `push 0xa; call
    // ??0CPtrList`) and the m_74 latch zeroes - exactly the inlined sequence at
    // BOTH retail construction sites (CMulti::SetupMultiplayerSession 0xb5460:
    // `push 0x78; call ??2` + 4x CPtrList(0xa) + `[esi+0x74]=0`; CPlay::Vfunc1
    // 0xc7ec0 likewise). m_70 is NOT initialized there - follow the bytes.
    CTileTriggerContainer() {
        m_74 = 0;
    }

    i32 DelFromList1(void* data); // 0x116e60
    // Scan m_list1 then m_list2 for the logic element with m_10 == a and
    // (b == 0 || m_typeTag == b).
    CTileTriggerLogic* FindInLists12(i32 a, i32 b); // 0x116f20
    i32 FilterList2(void* arg);                     // 0x1170b0
    i32 MoveList1ToList2(void* data);               // 0x117150
    i32 DelFromList3(void* data);                   // 0x117200

    // The /GX dtor: runs DtorBase then destroys m_list3 / m_list2 / m_list1 /
    // m_base (auto member teardown, reverse declaration order).
    ~CTileTriggerContainer(); // 0xc8640

    // The per-id logic-leaf factory (0x116610): switches on `logicType` (0x15..0x1a),
    // news the matching 0x9c CTileTriggerLogic leaf, fills it from the args + six
    // CTrigParam blocks (into m_block), appends it to m_list1 (or m_list2 for id 0x17),
    // and latches it into m_70 for id-0x15 board tiles 0x67/0x68.  Returns the leaf,
    // 0 on alloc/double-init.  /GX (per-`new` trylevel EH frame).
    CTileTriggerLogic* AddLogic(
        i32 a1,
        i32 logicType,
        i32 a3,
        i32 a4,
        i32 a5,
        CTrigParam p1,
        CTrigParam p2,
        CTrigParam p3,
        CTrigParam p4,
        CTrigParam p5,
        CTrigParam p6,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 a9
    );
    // 0x1163b0: forward to AddLogic with six default (zeroed) parameter blocks.
    void AddLogicDefaults(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9);
    // 0x1164a0: forward with the five ids + six CTrigParam blocks from a source record.
    void AddLogicFromRecord(i32 type, i32 a2, CTrigSourceRecord* rec);

    // Allocates a 0x28-byte CTileActionEvent, initialises it from the call args,
    // notifies it, and appends it to m_list3 (the +0x54 list).  /GX (the mark is a
    // stack-tracked partial during ctor).
    CTileActionEvent*
    AddToList3(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8); // 0x116a40

    // Allocates a 0xc8-byte CGiantRockLogic (type tag 0x16 == factory id 22), fills
    // it (incl. the 9-dword rep-movs m_matrix block) and appends it to m_list1 (the
    // +0x1c list).  /GX.
    CGiantRockLogic*
    AddToList1(i32 a1, i32 a2, i32 a3, i32* block9, i32 a5, i32 a6, i32 a7); // 0x116cf0 (matrix = ARG 4, retail-proven)

    // Twin of AddToList3 (0x116a40): allocates+constructs a 0x28-byte event, and
    // (when its init flag is clear) fills it from the args plus four state flags
    // chosen by a switch on `type` (0..5), notifies it, and appends it to m_list3.
    // Returns the event, or 0 on alloc/double-init failure.  /GX.
    CTileActionEvent* AddToList3Switch(i32 a1, i32 a2, i32 a3, i32 a4, i32 type); // 0x116b80

    // --- the walkers the thiscall-only tracer misfiled under CTileTriggerSwitchLogic ---
    // (their receiver is THIS container: ModeObjInit 0xc7ec0 builds it - four in-place
    // CPtrList ctors at +0x00/+0x1c/+0x38/+0x54 + [+0x74]=0 - and calls GetFlag74 on it;
    // TriggerMgr/BattlezMapConfig drive the finders and then DelFromList1/3 on the SAME
    // pointer; SetCell's "FindByKey @0x2838" / "AddMark @0x21df" / "RunFallback @0x377e"
    // ILT thunks resolve to FindByField0C / FindInLists12 / ScanNeighborhood.)
    i32 GetFlag74();                  // 0x115f00  test-and-set the m_74 latch
    i32 RemoveByKeys(i32 k1, i32 k2); // 0x116320  m_base: delete the (k1,k2) element
    // Scan m_base (the 0x8c switch-logic elements) for m_key1==k1 && (k2==0 || m_04==k2).
    CTileTriggerSwitchLogic* FindChild(i32 k1, i32 k2); // 0x116ee0
    // Scan m_list3 (the CTileActionEvent records) for m_c == key.
    CTileActionEvent* FindByField0C(i32 key); // 0x1171d0
    // 3x3 neighborhood probe around (x,y): FindInLists12((x'<<8)+y', 0x16) per
    // cell - tag 0x16 is the giant rock, so a hit IS a CGiantRockLogic.
    CGiantRockLogic* ScanNeighborhood(i32 x, i32 y); // 0x117ec0
    // The AddSwitchLogic factory stub (news a 0x8c CTileTriggerSwitchLogic; backlog).
    void AddSwitchLogic_115f60(); // 0x115f60

    // The big save/load serialize walk (0x117280).  op 4 = save: writes each list's
    // count and serialize-applies every element across m_base/m_list1/m_list2/m_list3
    // via SerializeApplyA/B and the per-element helpers.  op 7 = load: RemoveAll,
    // then for each list reads a count and LoadElement's that many elements, AddTail'd
    // into the matching list (m_list3 marks are alloc'd inline).  Returns 1/0.  /GX.
    i32 Serialize(CSerialArchive* s, i32 op, i32 a3, i32 a4); // 0x117280

    // Per-element LOAD helper of Serialize op 7: allocates+deserializes one element
    // and returns it (reloc-masked rel32 callee, 0x117800).  __thiscall on this.
    void* LoadElement(CSerialArchive* s, i32 op, i32 a3, i32 a4); // 0x117800

    // The serialize-walk pre/post hooks: stream the m_74 latch. LoadFlag74 closes
    // the load (op 7, read slot +0x2c); TransferFlag74 the save (op 4, write +0x30).
    i32 LoadFlag74(CSerialArchive* s);     // 0x117e70
    i32 TransferFlag74(CSerialArchive* s); // 0x117e20

    // Empties all four lists (m_base + m_list1/2/3), inline-destroying every
    // element, then clears m_70.  Invoked by DtorBase when m_74 is set.
    void RemoveAll(); // 0x116fa0

    // Looks up the CTileActionEvent for cell (a,b) via FindByField0C; if present
    // flags one (or all) of its m_playerFlags and re-commits it; else probes for a
    // covered-powerup command (FindInLists12 tag 0x1a) or scans the neighborhood.
    i32 SetCell(i32 a, i32 b, i32 verb); // 0x117f60

    // The base sub-object's own destructor; runs RemoveAll then clears +0x74.
    void DtorBase(); // 0x115f30

    TtcObList m_base;        // +0x00 (head @ +0x04)  the base CPtrList sub-object
    TtcObList m_list1;       // +0x1c (head @ +0x20)
    TtcObList m_list2;       // +0x38 (head @ +0x3c)
    TtcObList m_list3;       // +0x54 (head @ +0x58)
    CTileTriggerLogic* m_70; // +0x70  id-0x15 latches the built logic leaf here
    i32 m_74;                // +0x74  gates DtorBase's RemoveAll call, then cleared (0/nonzero)
};
SIZE_UNKNOWN(CTileTriggerContainer);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SRC_GRUNTZ_TILETRIGGERCONTAINER_H
