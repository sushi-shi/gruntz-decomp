# An MFC message map is two typed static objects, not an integer placeholder
tags: cpp:mfc cpp:static cpp:virtual | asm:mov | topic:mis-model topic:codegen-idiom topic:tooling
symptoms: BEGIN_MESSAGE_MAP AFX_MSGMAP AFX_MSGMAP_ENTRY first word points to CDialog::messageMap adjacent 24 zero bytes @@0QBU @@0PBU
confidence: 10/10

`BEGIN_MESSAGE_MAP` is an official MFC macro. For a derived dialog with no
handlers it emits an eight-byte `AFX_MSGMAP` followed by one 24-byte
`AFX_MSGMAP_ENTRY` terminator; the map fields relocate to the base class's map
and the derived class's entry array. A decimal first-word placeholder such as
`6205544` (`0x005eb068`) has lost that type and relocation.

```cpp
class CExampleDlg : public CDialog {
protected:
    static const AFX_MSGMAP messageMap;
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;

private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];
};

DATA(0x00123400)
const AFX_MSGMAP CExampleDlg::messageMap = {
    &CDialog::messageMap,
    &CExampleDlg::_messageEntries[0],
};

DATA(0x00123408)
const AFX_MSGMAP_ENTRY CExampleDlg::_messageEntries[] = {
    {0, 0, 0, 0, AfxSig_end, 0},
};
```

This is the relevant expansion of `BEGIN_MESSAGE_MAP`/`END_MESSAGE_MAP`, spelled
out only so the reconstructed data objects can carry `DATA(...)` and the
virtual declaration can carry the repository's required `OVERRIDE`. Use the
real declarations from `<Mfc.h>`; do not reproduce the structures or calling
conventions locally.

Tooling caveat: clang mangles the static entry-array candidate with
`@@0QBUAFX_MSGMAP_ENTRY@@B`, while VC5 emits
`@@0PBUAFX_MSGMAP_ENTRY@@B`. The label extractor may translate this one known
form only when the exact VC5 spelling exists in the compiled object. It must
not guess the rewrite for unrelated static data.

Evidence: `CMultiHelpDlg` retail data at `0x001ea448`/`0x001ea450` is an
eight-byte map with relocations to `CDialog::messageMap` and its own entries,
followed by one all-zero terminator. The typed expansion emits the same
payload/relocations and leaves `GetMessageMap` at `0x000bec00` byte-exact.
