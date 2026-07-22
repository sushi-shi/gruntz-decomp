#include <Gruntz/HaznColl.h> // shared coordinate/activation-registry collection
#include <Gruntz/GameRegMfcPtr.h>
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Wap32/ZVec.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/GruntzMgr.h>     // the REAL singleton class
#include <Gruntz/TileGrid.h>      // CTileGrid == CMapMgr (the +0x70 board's real class)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Bute/ButeMgr.h>         // CButeMgr (g_buteMgr GetIntDef), CButeTree (g_buteTree)
#include <Globals.h>
#include <Rez/FrameClock.h> // g_frameTime/g_engineFrameDelta (frame-clock band)

#include <Gruntz/AniElement.h> // CAniElement + CAniRecordView (the SetAnimEx idiom)

#include <DDrawMgr/DDrawSubMgrLeaf.h> // CDDrawSubMgrLeaf (m_world->m_animRegistry->m_10 cue lookup)

struct CHaznEntry; // an entry: first dword is the registered handler

VTBL(CStaticHazard, 0x001e7824);
DATA_SYMBOL(0x0024e3d0, 0x24, ?g_haznColl@@3UCCoordColl@@A)

RVA_COMPGEN(0x00012b30, 0x44, ??1CStaticHazard@@UAE@XZ)

RVA(0x000fbb70, 0x15)
void ConstructHaznRange() {
    g_haznColl.Construct(0x7d0, 0x7da);
}

#include <Gruntz/TypeKeyColl.h> // the REAL class at 0x6bf650 (its fields were the shredded g_type* globals)
struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)

#include <Gruntz/ActName.h> // CActName (shared)

static inline char* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return reinterpret_cast<char*>(
            (g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    if (reinterpret_cast<i32>(
            (static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0)
        )) { // slow lookup == _zvec::GrowTo @0x16da80
        return reinterpret_cast<char*>(
            (g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<char*>(g_typeColl.m_spare);
}

static inline CHaznEntry* HaznLookup(i32 coord) {
    return reinterpret_cast<CHaznEntry*>(g_haznColl.ResolveEntry(coord));
}

// ---------------------------------------------------------------------------
// CStaticHazard::~CStaticHazard @0x012b30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown; the
// destructible +0x18 link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CStaticHazard() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>

// ---------------------------------------------------------------------------
// CStaticHazard::CStaticHazard @0x0fb7a0 - the 1-arg ctor. Chains the standard
// CUserLogic(CGameObject*) leaf init (folded inline), then the static-hazard
// tail: re-arm the IDLE animation, snap the bound object to tile center, seed the
// pulse window from the "Hazardz/AniPad" bute int, cache the anim-set node.
//
// @early-stop
// jump-table + bound-object spill wall: logic byte-correct (the CUserLogic init,
// the IDLE re-arm via the SetAnimEx idiom, the tile snap (`and al,0xe0`), the
// 4-case level-kind jump table, the bbox/period seeding, the sound-map Lookup and
// the Hazardz/AniPad GetIntDef), but the dense bound-object field stores spill
// against retail's stack-slot schedule and the jump-table data region scores as
// the jumptable-data-overlap artifact. Parked for the final sweep.
RVA(0x000fb7a0, 0x2d4)
CStaticHazard::CStaticHazard(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    // re-arm the IDLE geometry + STATICHAZARD sprite (SetAnimEx idiom).
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
    {
        CAniElement* d = m_38->m_1a0.m_14;
        CAniRecordView* e =
            d->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(d->m_records.GetAt(0)) : 0;
        m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
    }
    // snap the bound object's screen position to tile center.
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_sortKey != 0) {
        m_object->m_sortKey = 0;
        m_object->m_flags |= 0x20000;
    }
    m_tileCol = m_object->m_screenX >> 5;
    m_tileRow = m_object->m_screenY >> 5;
    m_object->m_placeMode = 0;
    switch (g_gameReg->m_curState->m_levelType) {
        case 3:
        case 4:
        case 7:
        case 8:
            m_object->m_placeMode = m_object->m_screenY + 0x186b0;
            break;
        default:
            break;
    }
    m_object->m_area.left = m_object->m_screenX - 7;
    m_object->m_area.right = m_object->m_area.left + 14;
    m_object->m_area.top = m_object->m_screenY - 7;
    m_object->m_area.bottom = m_object->m_area.top + 14;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    m_object->m_1a0.m_2c = 0;
    m_object->m_124 = g_areaHazardParam;
    m_activeWindow = 0;
    m_idleWindow = m_object->m_120;
    m_pulseEpoch = g_frameTime;
    void* entry_ob = 0; // CMapStringToPtr::Lookup (0x1b8438) void*& out-param
    g_gameReg->m_world->m_animRegistry->m_10.Lookup("LEVEL_STATICHAZARDGO", entry_ob);
    CAniElement* entry = static_cast<CAniElement*>(entry_ob);
    if (entry != 0) {
        // base AniPad window + the resolved anim's frame total (the per-effect AniPad bias)
        m_activeWindow = g_buteMgr.GetIntDef("Hazardz", "AniPad", 0x64) + entry->m_total;
    } else {
        g_gameReg->ReportError(0x8009, 0x461);
    }
    if (m_object->m_120 == 0) {
        m_idleWindow = m_activeWindow;
    }
}

RVA(0x000fbbf0, 0x102)
void CStaticHazard::FireActivation(i32 coord) {
    CHaznEntry* e = HaznLookup(coord);
    if (e->m_fn != 0) {
        CHaznEntry* e2 = HaznLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CStaticHazard::RegisterActs @0x0fbd50 - intern "A" and "B" and bind each to its
// handler PMF (LoadAttributes2 @0xfc0b0 for "A", LoadAttributes @0xfc1a0 for "B")
// in the hazard registry. Two back-to-back single-key registrations; the SAME
// archetype as CTimeBomb::RegisterActs done twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x000fbd50, 0x2ac)
void CStaticHazard::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    HaznLookup(id)->m_fn = static_cast<HaznHandler>(&CStaticHazard::LoadAttributes2);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        id2 = g_typeCounter;
        g_buteTree.Insert("B", reinterpret_cast<void*>(id2));
        char* slot = ActNameLookup(id2);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    HaznLookup(id2)->m_fn = static_cast<HaznHandler>(&CStaticHazard::LoadAttributes);
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes2 @0x0fc0b0 - the time-gated pulse: bail when the
// registry is in the gated state; compute the running phase modulo the window;
// on a hit latch m_fired, re-arm the GO geometry/STATICHAZARD sprite (SetAnimEx
// idiom), and re-resolve the "B" anim-set node through the global bute tree.
//
// @early-stop
// store-vs-load scheduling wall (docs/patterns/statement-schedule-faithful.md):
// logic 100% correct, all reloc operands named. Retail emits `mov [m_fired],1` before
// reloading m_38; MSVC5 here hoists the m_38 load above the store. The m_38 reload
// is load-bearing (re-read per call), so the store/load pair can't be pinned.
// ~98%. Parked for the final sweep.
RVA(0x000fc0b0, 0xb2)
i32 CStaticHazard::LoadAttributes2() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1) {
        return 0;
    }
    u32 phase = g_frameTime - m_pulseEpoch;
    u32 base = static_cast<u32>(m_object->m_118);
    if (phase <= base) {
        return 0;
    }
    phase -= base;
    u32 span = m_idleWindow + m_activeWindow;
    if (phase % span > static_cast<u32>(m_activeWindow)) {
        return 0;
    }
    m_fired = 1;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
    {
        CAniElement* d = m_38->m_1a0.m_14;
        CAniRecordView* e =
            d->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(d->m_records.GetAt(0)) : 0;
        m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("B");
    return 0;
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes @0x0fc1a0 - the full periodic tick. Compute the
// running phase modulo the on/off window; depending on the window half + the
// fired flag (m_fired) + the bound object's gate (m_120), re-arm the GO or IDLE
// animation, drive the per-frame anim sub-object (SetAnim==2 => place + mark the
// hazard grid cell), and set/clear the cell's bit-0x8000000.
//
// @early-stop
// branch-heavy bound-object + grid-cell spill wall: logic byte-correct (the phase
// gate, the four re-arm paths, the SetAnim dispatch with the ScreenToCell/MarkCell
// calls, the grid set/clear-bit cell math), but the dense field reloads + the four
// grid-cell op sites spill against retail's stack-slot schedule. Parked for sweep.
RVA(0x000fc1a0, 0x33b)
i32 CStaticHazard::LoadAttributes() {
    u32 phase = (g_frameTime - m_pulseEpoch) - static_cast<u32>(m_object->m_118);
    u32 rem = phase % static_cast<u32>((m_idleWindow + m_activeWindow));
    if (rem > static_cast<u32>(m_activeWindow)) {
        // idle window
        if (m_fired == 0) {
            goto dispatch;
        }
        if (m_object->m_120 != 0) {
            // re-arm IDLE (cache the anim-set node first)
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = g_buteTree.Find("A");
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                CAniElement* d = m_38->m_1a0.m_14;
                CAniRecordView* e = d->m_records.GetSize() > 0
                                        ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                        : 0;
                m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
            }
            if (m_object->m_sortKey != 0) {
                m_object->m_sortKey = 0;
                m_object->m_flags |= 0x20000;
            }
            // clear the hazard cell's bit-0x8000000
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(m_tileCol) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(m_tileRow) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
            }
            return 0;
        }
        // m_120 == 0: re-arm GO + clear the fired flag
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            CAniElement* d = m_38->m_1a0.m_14;
            CAniRecordView* e = d->m_records.GetSize() > 0
                                    ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                    : 0;
            m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
        }
        if (m_object->m_sortKey != 0) {
            m_object->m_sortKey = 0;
            m_object->m_flags |= 0x20000;
        }
        m_fired = 0;
        return 0;
    } else {
        // active window
        if (m_fired != 0) {
            goto dispatch;
        }
        if (m_object->m_120 != 0) {
            goto dispatch;
        }
        // turn on: re-arm GO, latch the fired flag
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            CAniElement* d = m_38->m_1a0.m_14;
            CAniRecordView* e = d->m_records.GetSize() > 0
                                    ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                    : 0;
            m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
        }
        if (m_object->m_sortKey != 0) {
            m_object->m_sortKey = 0;
            m_object->m_flags |= 0x20000;
        }
        m_fired = 1;
        return 0;
    }

dispatch:
    if (m_38->m_1a0.Advance(g_engineFrameDelta) == 2) {
        i32 a = 0, b = 0;
        if (g_gameReg->m_cmdGrid->HitTestCell(m_object->m_screenX, m_object->m_screenY, &a, &b, 0)
            != 0) {
            g_gameReg->m_cmdGrid->CellDispatch(a, b, m_object->m_124, -1);
        }
        if (m_object->m_sortKey != m_object->m_placeMode) {
            m_object->m_sortKey = m_object->m_placeMode;
            m_object->m_flags |= 0x20000;
        }
        CTileGrid* grid = g_gameReg->m_tileGrid;
        if (static_cast<u32>(m_tileCol) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(m_tileRow) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[m_tileRow][m_tileCol * 7] |= 0x8000000;
        }
    } else {
        CTileGrid* grid = g_gameReg->m_tileGrid;
        if (static_cast<u32>(m_tileCol) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(m_tileRow) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
        }
        if (m_object->m_sortKey != 0) {
            m_object->m_sortKey = 0;
            m_object->m_flags |= 0x20000;
        }
    }
    {
        CAniAdvanceCursor* sub = &m_38->m_1a0;
        if (sub->m_28 != 0 && sub->m_20 == 0) {
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                CAniElement* d = m_38->m_1a0.m_14;
                CAniRecordView* e = d->m_records.GetSize() > 0
                                        ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                        : 0;
                m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_seedFrame);
            }
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(m_tileCol) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(m_tileRow) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>

RVA(0x000fc5b0, 0xf5)
i32 CStaticHazard::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* arc = static_cast<CSerialArchive*>(ar);
    switch (mode) {
        case 4:
            arc->Write(&m_pulseEpoch, 4);
            arc->Write(&m_activeWindow, 4);
            arc->Write(&m_idleWindow, 4);
            arc->Write(&m_fired, 4);
            arc->Write(&m_tileCol, 4);
            arc->Write(&m_tileRow, 4);
            break;
        case 7:
            arc->Read(&m_pulseEpoch, 4);
            arc->Read(&m_activeWindow, 4);
            arc->Read(&m_idleWindow, 4);
            arc->Read(&m_fired, 4);
            arc->Read(&m_tileCol, 4);
            arc->Read(&m_tileRow, 4);
            break;
    }
    if (!CUserLogic::SerializeMove(
            reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))),
            mode,
            a3,
            a4
        )) {
        return 0;
    }
    return Chain(arc, mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}
