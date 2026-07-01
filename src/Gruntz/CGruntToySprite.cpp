// CGruntToySprite.cpp - the "grunt has a toy" indicator sprite (C:\Proj\Gruntz).
// A CUserLogic-derived game object; methods in ascending retail-RVA order:
//   ~CGruntToySprite  @0x0122b0 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell           @0x07f920 - stash the (x,y) grunt cell, clear m_38 bit 0.
//   Update            @0x07f960 - track the grunt's screen pos + layer.
//
// The 0x44 is a DESTRUCTOR (stamps CUserLogic 0x5e705c then CUserBase 0x5e70b4,
// tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a ctor - identical in
// shape to ~CTimeBomb @0x012a70.
#include <Gruntz/CGruntToySprite.h>

// ~CGruntToySprite @0x0122b0 - the CUserLogic-folded /GX leaf dtor (see header).
RVA(0x000122b0, 0x44)
CGruntToySprite::~CGruntToySprite() {}

// CGruntToySprite::InitActReg @0x07f540 - construct the class's activation-
// coordinate registry singleton (g_toyActReg @0x644d58) over [2000, 2010] via
// the shared registry ctor (FUN_00408710). Free init thunk; reloc-masked.
RVA(0x0007f540, 0x15)
void CGruntToySprite::InitActReg() {
    g_toyActReg.Construct(2000, 2010);
}

// CGruntToySprite::RegisterActs @0x07f720 - bind the class's per-frame handler
// (Update @0x07f960) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0007f720, 0x18d)
void CGruntToySprite::RegisterActs() {
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
    ((CToyActEntry*)g_toyActReg.ResolveEntry(id))->m_fn = &CGruntToySprite::Update;
}

// SetCell @0x07f920 - stash the (x,y) grunt cell, clear bit 0 of the +0x38 game
// object's +0x40 flags, return 1.
RVA(0x0007f920, 0x21)
i32 CGruntToySprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    m_38->m_40 &= ~1;
    return 1;
}

// Update @0x07f960 - resolve the grunt for cell (m_cellX,m_cellY); when present, if its
// layer index changed re-clamp it through the level layer table into the bound
// renderable's layer fields, then copy the grunt's screen position (y biased by
// -0x20) into the bound renderable. Returns 0.
RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGruntEntry* e = ((CGruntEntry**)(g_gameReg->m_68 + 0x1c))[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_198;
    if (m_lastLayer != layer) {
        CGruntRenderable* r = (CGruntRenderable*)m_10;
        m_lastLayer = layer;
        CGruntLayerHolder* h = r->m_194;
        if (h != 0) {
            i32 mapped;
            if (layer >= h->m_64 && layer <= h->m_68) {
                mapped = h->m_14[layer];
            } else {
                mapped = 0;
            }
            r->m_198 = mapped;
            r->m_190 = layer;
        }
    }
    m_10->m_5c = e->m_10->m_5c;
    m_10->m_60 = e->m_10->m_60 - 0x20;
    return 0;
}

// class-metadata sweep: grunt/game-object family size annotations (SIZE_UNKNOWN = retail size TBD, at .cpp EOF).
SIZE_UNKNOWN(CGruntToySprite);
SIZE_UNKNOWN(CToyActEntry);
