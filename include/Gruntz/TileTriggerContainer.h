// TileTriggerContainer.h - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 3-CObList container: a base sub-object at +0x00 plus three CObList members
// at +0x1c / +0x38 / +0x54 (their m_pNodeHead at +0x20 / +0x3c / +0x58).  The
// lists hold heap command objects; the accessors here move/find/remove those
// objects across the three lists.  The list elements are sibling command objects
// (the CTileGridCommand class) destroyed inline (vtable 0x5eaea4 stamp + RezFree)
// before the node is unlinked.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILETRIGGERCONTAINER_H
#define SRC_GRUNTZ_TILETRIGGERCONTAINER_H

#include <Mfc.h> // MFC CObList (the m_base/m_list1-3 sub-object list methods)
#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <rva.h>                  // SIZE_UNKNOWN class-metadata macros used below

// The owning container; back-stamped into the list elements (m_14 / m_20).
class CTileTriggerContainer;

// The Rez heap alloc/free (RVA 0x1b9b46 _RezAlloc / 0x1b9b82 _RezFree);
// reloc-masked rel32 callees.
extern "C" void* RezAlloc(u32 n);
extern "C" void RezFree(void* p);

// The running game clock (DAT_00645588); reloc-masked DIR32 datum.
extern "C" u32 g_645588;

// A list element (command) object's vftable, stamped into the element by the
// inlined element-destructor before RezFree.  Reloc-masked DIR32 datum.

// The base-list element vftable (the +0x00 list's elements, cleared at elem+0x20
// before free).  Reloc-masked DIR32 datum.

// A keyed element found by SetCell's lookup: a 4-slot state-flag array at
// +0x18..+0x24 and a slot-0 Notify (called with its own vtable pointer).
// __thiscall callee.
struct TtcKeyedElem {
    virtual void Slot0(); // vptr @+0x00 (real polymorphic; declared-only)
    char _pad04[0x18 - 0x04];
    i32 m_flags[4]; // +0x18..+0x24  state flags [0..3]
};
SIZE_UNKNOWN(TtcKeyedElem);

// A singly-walked CObList node: next@+0x00, data@+0x08 (MFC CList layout, the
// +0x04 prev slot unused by these walkers).
struct TtcNode {
    TtcNode* m_next; // +0x00
    char _pad04[4];  // +0x04 (prev)
    // +0x08  the CObList node payload: a genuine heterogeneous CObject* slot - the
    // four lists store DIFFERENT element types (TtcElem / TtcMark / TtcTrigElem /
    // plain i32 records), downcast per walker. Authentic void* (MFC container payload).
    void* m_data;
};
SIZE_UNKNOWN(TtcNode);

// One MFC CObList sub-object (0x1c bytes): only m_pNodeHead (+0x04) is read by
// the walkers; RemoveAt / AddTail are the reloc-masked rel32 callees.
class TtcObList {
public:
    void RemoveAt(void* pos); // 0x1b4ac7
    void* AddTail(void* obj); // 0x1b4991
    void RemoveAll();         // 0x1b48a6
    void Dtor();              // 0x1b48c6  ~CObList (reloc-masked rel32 callee)
    virtual ~TtcObList() {
        Dtor();
    } // real virtual subobject dtor @ vptr +0x00: drives the container's /GX frame
    TtcNode* m_pNodeHead;     // +0x04
    char _pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                 // +0x0c  element count (serialized by 117280)
    char _pad10[0x1c - 0x10]; // +0x10..0x1b
};
SIZE_UNKNOWN(TtcObList);

// The list3 (m_list3, +0x54) element: a 0x28-byte record initialised from the
// AddToList3 args, notified, then appended.  m_10 gates the init (1 = live).
struct TtcMark {
    // Per-mark (de)serialize-and-fill helper used by the big serialize walk
    // (0x117280): reads the mark's fields from the stream and validates them.
    // __thiscall, returns nonzero on success.  Reloc-masked rel32 callee (0x113f10).
    i32 m_00;
    i32 m_04;
    i32 m_08;
    i32 m_0c;
    i32 m_10;                    // +0x10  init flag
    CTileTriggerContainer* m_14; // +0x14  owning container
    i32 m_18;
    i32 m_1c;
    i32 m_20;
    i32 m_24;
};
SIZE_UNKNOWN(TtcMark);
// The mark allocator/ctor: RezAlloc(0x28) then the __thiscall ctor (0x1e1a).
extern "C" TtcMark* TtcMarkCtor(TtcMark* p); // 0x1e1a (reloc-masked)

// The list1 (m_list1, +0x1c) element: a 0xc8-byte command record initialised from
// the AddToList1 args (incl. a 9-dword rep-movs block at +0x9c), a type tag 0x16
// at +0x04, two game-clock snapshots at +0x24.  m_1c gates the init.
struct TtcBaseElem {
    i32 m_00;
    i32 m_04; // +0x04  type tag (0x16)
    i32 m_08;
    i32 m_0c;
    i32* m_10; // +0x10  the 9-dword source block (AddToList1's block9 arg)
    i32 m_14;
    i32 m_18;
    i32 m_1c;                    // +0x1c  init flag
    CTileTriggerContainer* m_20; // +0x20  owning container
    i32 m_24;                    // +0x24  clock snapshot
    i32 m_28;
    i32 m_2c;
    i32 m_30;
    i32 m_34;
    i32 m_38;
    char _pad3c[0x9c - 0x3c];
    i32 m_9c[9]; // +0x9c  9-dword block (rep movs)
    i32 m_c0;    // +0xc0
    i32 m_c4;    // +0xc4
};
SIZE_UNKNOWN(TtcBaseElem);
extern "C" TtcBaseElem* TtcBaseElemCtor(TtcBaseElem* p); // 0x2c3e (reloc-masked)

// The serialize helpers operate on the factory-built trigger-logic element - a
// CTrigLogic (the object CTileTriggerFactory::Build @0x117800 Rez-allocates,
// constructs, and registers).  Its +0x04 is the serialized type TAG that drives the
// dispatch below; the appliers reached here are CTrigLogic's OWN register thunks
// Reg277f (0x277f) / Reg1d39 (0x1d39) / Reg1abe (0x1abe) - the very thunks the
// factory calls (confirmed by the caller graph: each of 0x277f / 0x1d39 / 0x1abe is
// called by both 0x117630 / 0x117710 here AND 0x117800 Build).  Reloc-masked
// __thiscall callees returning nonzero on success.
//
// Modeled as an honest local view (NOT CTileTriggerSwitchLogic - a different class /
// layout, vtable 0x5eae8c; the earlier TtcSwitchObj->CTileTriggerSwitchLogic fold was
// a mis-attribution).  Correctness-not-artifacts: an honest placeholder named for its
// true CTrigLogic identity beats a wrong class name.
struct TtcTrigElem {
    // Reg277f @0x277f IS Gate113860 (free __stdcall); call it, drop the receiver.
    // Reg1d39 @0x277f IS Gate113860 (free __stdcall); call it, drop the receiver.
    // Reg1abe @0x277f IS Gate113860 (free __stdcall); call it, drop the receiver.
    i32 m_00; // +0x00
    i32 m_04; // +0x04  serialized type tag (the factory's switch id)
};
SIZE_UNKNOWN(TtcTrigElem);

// The two tag-dispatched serialize-and-apply helpers of 117280.  __stdcall free
// functions: stream the element's tag, then dispatch to its register thunks.
i32 __stdcall
SerializeApplyA(CSerialArchive* s, i32 a2, i32 a3, i32 a4, TtcTrigElem* o); // 0x117630
i32 __stdcall
SerializeApplyB(CSerialArchive* s, i32 a2, i32 a3, i32 a4, TtcTrigElem* o); // 0x117710

class CTileTriggerContainer {
public:
    i32 DelFromList1(void* data);      // 0x116e60
    void* FindInLists12(i32 a, i32 b); // 0x116f20
    i32 FilterList2(void* arg);        // 0x1170b0
    i32 MoveList1ToList2(void* data);  // 0x117150
    i32 DelFromList3(void* data);      // 0x117200

    // The /GX dtor: runs DtorBase then destroys m_list3 / m_list2 / m_list1 /
    // m_base (auto member teardown, reverse declaration order).
    ~CTileTriggerContainer(); // 0xc8640

    // Allocates a 0x28-byte mark, initialises it from the call args, notifies it,
    // and appends it to m_list3 (the +0x54 list).  /GX (the mark is a stack-tracked
    // partial during ctor).
    TtcMark* AddToList3(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8); // 0x116a40

    // Allocates a 0xc8-byte element, fills it (incl. a 9-dword rep-movs block) and
    // appends it to m_list1 (the +0x1c list).  /GX.
    TtcBaseElem*
    AddToList1(i32 a1, i32 a2, i32* block9, i32 a4, i32 a5, i32 a6, i32 a7); // 0x116cf0

    // Twin of AddToList3 (0x116a40): allocates+constructs a 0x28-byte mark, and
    // (when its init flag is clear) fills it from the args plus four state flags
    // chosen by a switch on `type` (0..5), notifies it, and appends it to m_list3.
    // Returns the mark, or 0 on alloc/double-init failure.  /GX.
    TtcMark* AddToList3Switch(i32 a1, i32 a2, i32 a3, i32 a4, i32 type); // 0x116b80

    // The big save/load serialize walk (0x117280).  op 4 = save: writes each list's
    // count and serialize-applies every element across m_base/m_list1/m_list2/m_list3
    // via SerializeApplyA/B and the per-element helpers.  op 7 = load: RemoveAll,
    // then for each list reads a count and LoadElement's that many elements, AddTail'd
    // into the matching list (m_list3 marks are alloc'd inline).  Returns 1/0.  /GX.
    i32 Serialize(CSerialArchive* s, i32 op, i32 a3, i32 a4); // 0x117280

    // Per-element LOAD helper of Serialize op 7: allocates+deserializes one element
    // and returns it (reloc-masked rel32 callee, 0x117800).  __thiscall on this.
    void* LoadElement(CSerialArchive* s, i32 op, i32 a3, i32 a4); // 0x117800

    // The serialize-walk pre/post hooks (reloc-masked rel32 callees).  LoadFlag74
    // closes the load (op 7); TransferFlag74 closes the save (op 4).  __thiscall.
    // (Real fn: CTileTriggerSwitchLogic::LoadFlag74 / ::TransferFlag74; called on this.)
    i32 LoadFlag74(CSerialArchive* s);     // 0x117e70
    i32 TransferFlag74(CSerialArchive* s); // 0x117e20

    // Empties all four lists (m_base + m_list1/2/3), inline-destroying every
    // element, then clears m_70.  Invoked by DtorBase when m_74 is set.
    void RemoveAll(); // 0x116fa0

    // Looks up the keyed element for cell (a,b); if present flags one (or all)
    // of its +0x18..+0x24 state words and notifies it; else registers a new mark
    // (helper 0x21df) or runs the fallback (helper 0x377e).  Returns 1/0.
    i32 SetCell(i32 a, i32 b, i32 verb); // 0x117f60
    TtcKeyedElem* FindByKey(i32 key);    // 0x2838 (reloc-masked)
    void* AddMark(i32 key, i32 kind);    // 0x21df (reloc-masked)
    i32 RunFallback(i32 a, i32 b);       // 0x377e (reloc-masked)

    // The base sub-object's own destructor; runs RemoveAll then clears +0x74.
    void DtorBase(); // 0x115f30

    TtcObList m_base;  // +0x00 (head @ +0x04)  the base CObList sub-object
    TtcObList m_list1; // +0x1c (head @ +0x20)
    TtcObList m_list2; // +0x38 (head @ +0x3c)
    TtcObList m_list3; // +0x54 (head @ +0x58)
    i32 m_70;          // +0x70
    i32 m_74;          // +0x74  gates DtorBase's RemoveAll call, then cleared (0/nonzero)
};
SIZE_UNKNOWN(CTileTriggerContainer);

#endif // SRC_GRUNTZ_TILETRIGGERCONTAINER_H
