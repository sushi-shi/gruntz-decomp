# Use MFC's message-map macros and label their generated objects

tags: cpp:mfc cpp:macro cpp:static cpp:virtual | asm:mov | topic:mis-model topic:codegen-idiom topic:tooling
symptoms: BEGIN_MESSAGE_MAP AFX_MSGMAP AFX_MSGMAP_ENTRY GetMessageMap hand-expanded handler records
confidence: 10/10

`DECLARE_MESSAGE_MAP`, `BEGIN_MESSAGE_MAP`, the `ON_*` entries and
`END_MESSAGE_MAP` are the source layer for the six reconstructed dialogs.
Static MFC's implementation emits a six-byte getter, an eight-byte map and a
24-byte record per handler, including an all-zero final record. The map's
ordered pointers name the base map and the class's own entry array.

Use the SDK macros rather than reproducing their declarations or initializer
records. The SDK owns `GetMessageMap`'s protected virtual declaration (and
therefore its override slot), `messageMap` and `_messageEntries`. An invented
`s_` prefix or public access changes their mangled names without improving the
source model.

```cpp
class CExampleDlg : public CDialog {
    DECLARE_MESSAGE_MAP()
};

RVA(0x00123000, 0x6)
DATA_MESSAGE_MAP(0x00123400, 0x00123408)
BEGIN_MESSAGE_MAP(CExampleDlg, CDialog)
END_MESSAGE_MAP()
```

`RVA` annotates the SDK-generated getter. `DATA_MESSAGE_MAP` attaches the two
data addresses to that same definition; the extractor uses its class scope to
select the actual main-file VarDecls, their linkage, definition status and
extents. It derives VC5 names from Clang's declarations, including the known
array-storage `Q` to `P` rewrite. It never accepts an absent member by inventing
a symbol. Both data addresses participate in the completeness sweep and the
ordinary `src` data channel.

Keep the macro block at the getter's retail text position. Moving a formerly
separate getter to the start of the TU would change function emission order.

## Clang compatibility is separate from matching source

VC5 accepts the SDK's implicit `&OnPaint`, `&OnTimer`, `&OnMeasureItem` and
`&OnDrawItem` inside the static array initializer. Clang rejects these with
“must explicitly qualify name of member function when taking its address”.
This is a parser compatibility defect, not evidence against the SDK macros.

`gruntz.graph.compdb` makes a regular-file copy of `AFXMSG_.H` in the generated
Clang lowercase mirror and qualifies its implicit `&On...` expressions with
`MfcMessageMapClass::`. All SDK message IDs, signature tags and cast expressions
remain intact. A class using these entries provides
`MFC_MESSAGE_MAP_CLASS(CExampleDlg)` beside `DECLARE_MESSAGE_MAP()`: this is a
Clang-only self-type alias and expands to nothing in VC5. The pinned SDK is
untouched; the matching compiler reads the original header.

## Controlled evidence and reverse audit

The 2026-09-24 audit compared the old expansions, restored SDK macros and raw
retail bytes for all six getters and all twelve data objects. All non-relocated
bytes, ordered DIR32 targets and addends agree. Handler pointers were checked
through retail's incremental-link thunks, not merely compared with masked
objdiff bytes. The 65 entry records include 59 handlers and six terminators.
See [the search audit](../mfc-macro-audit.md) for the complete family.

The original version of this pattern claimed manual expansion was necessary
for `DATA` and `OVERRIDE`. That was a tooling restriction, not an original-source
fact. `DATA_MESSAGE_MAP` removes the label restriction; `DECLARE_MESSAGE_MAP`
provides the SDK's own virtual declaration. The old explanation must not be
used to justify another hand-expanded map.

Detection: search for `AFX_MSGMAP`, `AFX_MSGMAP_ENTRY`, `AfxSig_*`, a getter
returning a static map, or erased handler-pointer casts. Recover the complete
macro family, keep ordered entries (even duplicate handlers), then compare raw
bytes and targets. Do not infer `ON_COMMAND` versus an equivalent notification
macro from bytes alone. Do not infer assertion/debug macros erased by release
compilation from their absence.

`gruntz verify selftest -k MessageMap` exercises real SDK preprocessing through
IR, layout and final source-claim extraction for two maps in one TU and a
neighboring `DATA` declaration. Negative controls reject a wrong carrier,
missing definitions, a preprocessed-out annotation and a missing second data
claim. A mirror control proves the SDK file remains unchanged and regeneration
is idempotent.
