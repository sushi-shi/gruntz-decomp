// TileTriggerSwitchLogic.cpp - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// CTileTriggerSwitchLogic is a tile-trigger "switch" class that manages a linked
// list (anchor at +0x04) of owned sibling CTileTriggerSwitchLogic objects.  The
// ctor @0x110430 zeroes m_block and seeds m_20; the method at 0x116320 searches
// the list for a matching (m_04, m_10) pair and removes/deletes the found node.
//
// vftable @0x5eae8c.
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include "TileTriggerSwitchLogic.h"

// The vftable for this class, referenced as a DIR32 extern.
// @data: 0x1eae8c
extern void *g_tileTriggerSwitchLogicVtbl;   // VA 0x5eae8c

// operator new / operator delete (global, @0x1b9b46 / @0x1b9b82).
void *operator new(size_t);
void operator delete(void *);

// Linked-list node-unlink helper @0x1b4ac7.  __thiscall on this (the list owner
// = CTileTriggerSwitchLogic whose m_04 is the head), node ptr on the stack.
// External/no-body; call displacement is reloc-masked.
void __fastcall ListUnlink(CTileTriggerSwitchLogic *listOwner, void *node);

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()  @0x110430
// Constructor: stamps the vtable, zeroes the 24-dword m_block at +0x2c
// (rep stosd), then clears m_20 (+0x20).
//
// @address: 0x110430
// @size:    0x1c
// ---------------------------------------------------------------------------
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()
{
    for (int i = 0; i < 24; i++)
        m_block[i] = 0;
    m_20 = 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::RemoveByKey  @0x116320
// Traverses the singly-linked list anchored at m_04 (each node: next@0x00,
// data@0x08).  For each node, compares the data object's m_04 to key2 and
// m_10 to key1.  On match, re-stamps the data's vtable, clears m_20, frees
// the data (operator delete), unlinks the node from the list, and returns 1.
// Returns 0 if no match found or if the list is empty.
//
// @address: 0x116320
// @size:    0x66
// ---------------------------------------------------------------------------
int CTileTriggerSwitchLogic::RemoveByKey(int key1, int key2)
{
    ListNode *cur = (ListNode *)m_04;
    if (cur == 0)
        return 0;

    do {
        CTileTriggerSwitchLogic *data = cur->m_data;
        if (data->m_04 == key2 && data->m_10 == key1) {
            if (data != 0) {
                *(void **)data = &g_tileTriggerSwitchLogicVtbl;
                data->m_20 = 0;
                operator delete(data);
            }
            ListUnlink(this, cur);
            return 1;
        }
        cur = cur->m_next;
    } while (cur != 0);

    return 0;
}
