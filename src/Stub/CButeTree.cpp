#include <Bute/ButeTree.h> // canonical CButeTree (one shape)
#include <rva.h>
// CButeTree.cpp - engine-label stub for CButeTree.
//
// The shared find-by-key store node every CButeMgr getter funnels through
// (outer Find(tag) on m_tree, inner Find(key) on the tag sub-tree); CButeTree is
// the canonical crit-bit trie (include/Bute/ButeTree.h).
//
// The global bute store instance the game-object ctors query for their "A" node
// (0x6bf620 -> DATA rva 0x2bf620). Shared across the Stub TU; declared here so
// every game-object ctor's `g_buteTree.Find("A")` binds the one symbol.
DATA(0x002bf620)
extern CButeTree g_buteTree;
