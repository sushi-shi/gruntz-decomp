# Const-cast contracts

This directory documents every remaining `const_cast` expression in `src/` and
`include/`, and four repaired sites. Each site has its own page: our code, the declarations that conflict, source
and Git provenance, a GitHub comparison, the safety evidence and its limits,
and what would allow the cast to be removed.

These are current contract reviews, not blanket approvals. A cast being needed
by our present declarations does not prove those declarations are correct.
Examples from other projects demonstrate usage; they do not by themselves
prove that a vendor never writes through a pointer.

## All current sites

| Site | Our expression or boundary | Contract review |
| --- | --- | --- |
| `CNetMgr::CreateSession` | `const_cast<char*>(password)` | [DirectPlay password](directplay-password.md) |
| `CNetMgr::CreatePlayer` | `const_cast<char*>(shortName)` | [DirectPlay short name](directplay-short-name.md) |
| `CNetMgr::CreatePlayer` | `const_cast<char*>(longName)` | [DirectPlay long name](directplay-long-name.md) |
| `zErrHandling::handle` | `const_cast<zErrHandling*>(this)` | [Error-handler identity](ztools-error-handle.md) |
| `zErrHandling::handle_inl` | `const_cast<zErrHandling*>(this)` | [Inline error-handler identity](ztools-error-handle-inl.md) |
| `zBitVec::body` | `const_cast<u32*>(&m_inline)` | [Mutable bitset accessor](ztools-bitvec-body.md) |
| `CChatBox::HandleTextInputKey` | Repaired: writable owned CString copy | [Game chat input](chat-input.md) |
| `CMultiStartDlg::OnChatSend` | Repaired: CString writable-buffer protocol | [Lobby chat message](chat-dialog.md) |
| `CMenuSparkle::SerializeDispatch` | Repaired: load lower bound into writable temporary | [Sparkle lower bound](menu-sparkle-low.md) |
| `CMenuSparkle::SerializeDispatch` | Repaired: load upper bound into writable temporary | [Sparkle upper bound](menu-sparkle-high.md) |

The repeated wrappers and paired fields are separate sites even when they
share an explanation. There are six remaining expressions and ten pages,
including the four removed casts and their replacement contracts.

## How to read the evidence

- **SDK declaration:** the actual type supplied by the toolchain. The DirectPlay
  records have mutable string fields; MFC's CString conversion gives a const
  view. Neither fact identifies an original Gruntz function signature.
- **Related library source:** the surviving NOLF ZTools header contains the
  const wrappers and mutable-returning const accessor. Its exact Git blob
  matches the [lineage ledger](../../config/lithtech_lineage.tsv). This is
  stronger provenance than a similar unrelated GitHub example, but still needs
  adaptation to the Gruntz owner and storage model.
- **Implementation evidence:** an inspected implementation, such as a pinned
  Wine revision, can show what that implementation does. It is not a universal
  guarantee about Microsoft's DirectPlay DLLs or all versions of a library.
- **Retail binary evidence:** actual writes, referents, storage protection and
  calling behavior constrain the reconstruction. Read-only placement alone
  does not uniquely recover a C++ qualifier. See
  [qualifiers and memory protection](../data-attribution.md#qualifiers-and-memory-protection).

Removing constness from a pointer does not itself modify its target. A write
to an object defined as const is undefined behavior; using a const view of
mutable storage is a different case. CString's ownership and cached-length
rules also matter even when the underlying character allocation is mutable.
The MenuSparkle pages document why the original read destinations were unsafe
and how the repaired loads consume the stored values without writing constants.

## Maintaining the inventory

Run this from the repository root when adding, removing or changing a cast:

```sh
rg -n 'const_cast' src include
```

Check each resulting expression against the table and its page. Update both
ends of a changed contract, including upstream callers and any vendor record.
Use symbol names as the primary identity; source line numbers in links can
move. Keep GitHub comparisons pinned to full commits, identify omitted code in
excerpts, and label alternatives as proposals rather than changes already made.

For provenance, follow `git blame` with `git log -S` or the introducing diff
when blame only identifies a rename, formatting change or squash commit. Record
why the qualifier and cast were introduced separately. Keep source-adoption
decisions in the lineage ledger; these pages explain the current boundaries.
