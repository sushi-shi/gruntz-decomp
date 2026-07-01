// CGruntHealthSprite.cpp - the grunt health-bar eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntHealthSprite methods, defined in ascending retail-RVA order:
//   ~CGruntHealthSprite @0x011fb0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   InitActReg          @0x07ecf0 - construct the class activation registry.
//   RegisterActs        @0x07eed0 - register the class's per-frame handler.
//   SetHealthGlyph      @0x07f0d0 - the per-bump health-glyph resolver (ret 0xc).
//
// CGruntHealthSprite : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CGruntHealthSprite.h>

// CGruntHealthSprite::~CGruntHealthSprite @0x011fb0 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00011fb0, 0x44)
CGruntHealthSprite::~CGruntHealthSprite() {}

// CGruntHealthSprite::InitActReg @0x07ecf0 - construct the class's activation-
// coordinate registry singleton (g_healthActReg @0x644d80) over the fixed range
// [2000, 2010] via the shared registry ctor (FUN_00408710). A free init thunk
// (no `this`); the ctor is reloc-masked.
RVA(0x0007ecf0, 0x15)
void CGruntHealthSprite::InitActReg() {
    g_healthActReg.Construct(2000, 2010);
}

// CGruntHealthSprite::RegisterActs @0x07eed0 - bind the class's per-frame handler
// (HealthUpdate @0x07f180) to the activation key "A". The key is first resolved
// to an id via the bute-tree name map (g_buteTree.Find); a fresh key gets the
// next id (g_buteTree.Insert), is interned into the shared name registry slot
// (ActNameLookup -> free the old name list, assign "A"), and bumps the counter.
// The id is then resolved to an entry in the class registry and the handler PMF
// stored there.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc): the logic is byte-faithful and the instruction SELECTION is
// exact end-to-end - the bute Find/Insert, the shared-name-registry resolve, the
// inlined CString-list free loop (the `while (n-- != 0)` spelling reproduces
// retail's `mov eax,N; mov ecx,eax; dec eax; test ecx; je; lea ebp,[eax+1]`
// count-recover per test-old-value-decrement-loop-while-postdec.md +
// predecrement-guard-lea-recover-count.md), operator=, the id-bump, and the
// per-class id->entry resolve with the `mov [entry],offset HealthUpdate` handler
// store. ~93%; the SOLE residual is which callee-saved register holds the name
// slot vs the live id - retail pins the slot in esi / id in edi, cl swaps them
// (slot->edi / id->esi), a whole-function allocation choice not source-steerable.
// Logic complete; deferred to the final sweep.
RVA(0x0007eed0, 0x18d)
void CGruntHealthSprite::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CHealthActEntry*)g_healthActReg.ResolveEntry(id))->m_fn = &CGruntHealthSprite::HealthUpdate;
}

// CGruntHealthSprite::SetHealthGlyph @0x07f0d0 - stash the two passed coordinates
// (m_cellX/m_cellY), round the passed health to a glyph slot (slot = 0x15 -
// (int)(health*0.2 + 0.5); the *0.2+0.5 round emits fild/fmul[0.2]/fadd[0.5]/
// __ftol), resolve that slot through the bound object's [m_64..m_68]-gated glyph
// table at +0x194, publish the glyph (+0x198) and slot (+0x190) back into the
// object, stash the health (m_health), return 1.
RVA(0x0007f0d0, 0x6e)
i32 CGruntHealthSprite::SetHealthGlyph(i32 x, i32 y, i32 health) {
    m_cellX = x;
    m_cellY = y;
    i32 slot = 0x15 - (i32)((double)health * 0.2 + 0.5);
    CHealthSpriteObj* obj = (CHealthSpriteObj*)m_10;
    CHealthGlyphMap* map = obj->m_194;
    if (map) {
        i32 glyph;
        if (slot >= map->m_64 && slot <= map->m_68) {
            glyph = map->m_14[slot];
        } else {
            glyph = 0;
        }
        obj->m_198 = glyph;
        obj->m_190 = slot;
    }
    m_health = health;
    return 1;
}

// class-metadata sweep: grunt/game-object family size annotations (SIZE_UNKNOWN = retail size TBD, at .cpp EOF).
SIZE_UNKNOWN(CGruntHealthSprite);
SIZE_UNKNOWN(CHealthActEntry);
SIZE_UNKNOWN(CHealthGlyphMap);
SIZE_UNKNOWN(CHealthSpriteObj);
