"""Real SDK -> Clang IR/layout -> source-claim controls for message maps."""
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from gruntz.retail_labels import source
from gruntz.tool import clang


class MessageMapControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        db = clang.compdb()
        if not db:
            raise unittest.SkipTest("run gruntz init/build for the pinned SDK compdb")
        cls.flags = next(iter(db.values()))

    def extract(self, body):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            path = root / "Probe.cpp"
            path.write_text('#include <StdAfx.h>\n#include <rva.h>\n' + body)
            with mock.patch.object(source, "REPO", root):
                return source.extract_unit("probe", "Probe.cpp", {str(path): self.flags})

    def test_real_sdk_macros_keep_two_maps_and_neighboring_data_separate(self):
        rows, errors = self.extract('''
class CEmptyProbe : public CDialog {
    DECLARE_MESSAGE_MAP()
};
class CHandlerProbe : public CDialog {
public:
    void OnPaint() {}
    void OnTimer(UINT) {}
    void OnMeasureItem(int, LPMEASUREITEMSTRUCT) {}
    void OnDrawItem(int, LPDRAWITEMSTRUCT) {}
    MFC_MESSAGE_MAP_CLASS(CHandlerProbe)
    DECLARE_MESSAGE_MAP()
};
RVA(0x00010000, 0x6)
DATA_MESSAGE_MAP(0x00200000, 0x00200008)
BEGIN_MESSAGE_MAP(CEmptyProbe, CDialog)
END_MESSAGE_MAP()
RVA(0x00010010, 0x6)
DATA_MESSAGE_MAP(0x00200100, 0x00200108)
BEGIN_MESSAGE_MAP(CHandlerProbe, CDialog)
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
END_MESSAGE_MAP()
DATA(0x00200200)
int g_neighbor;
// DATA_MESSAGE_MAP(0x00200300, 0x00200308)
''')
        self.assertEqual(errors, [])
        data = {r[0]: (r[1], r[2]) for r in rows if r[3] == "data"}
        self.assertEqual(data, {
            "0x00200000": ("0x8", "?messageMap@CEmptyProbe@@1UAFX_MSGMAP@@B"),
            "0x00200008": ("0x18", "?_messageEntries@CEmptyProbe@@0PBUAFX_MSGMAP_ENTRY@@B"),
            "0x00200100": ("0x8", "?messageMap@CHandlerProbe@@1UAFX_MSGMAP@@B"),
            "0x00200108": ("0x78", "?_messageEntries@CHandlerProbe@@0PBUAFX_MSGMAP_ENTRY@@B"),
            "0x00200200": ("0x4", "?g_neighbor@@3HA"),
        })
        self.assertEqual({r[0] for r in rows if r[3] == "func"},
                         {"0x00010000", "0x00010010"})
        self.assertTrue(all(r[4] == "src" for r in rows))

    def test_wrong_annotation_carrier_fails_extraction(self):
        rows, errors = self.extract('''
DATA_MESSAGE_MAP(0x00200000, 0x00200008)
int NotAMessageMap() { return 0; }
''')
        self.assertFalse(any(r[3] == "data" for r in rows))
        self.assertTrue(any("not an MFC GetMessageMap" in e for e in errors), errors)

    def test_header_declarations_cannot_supply_missing_definitions(self):
        rows, errors = self.extract('''
class CMissingProbe : public CDialog { DECLARE_MESSAGE_MAP() };
RVA(0x00010000, 0x6)
DATA_MESSAGE_MAP(0x00200000, 0x00200008)
const AFX_MSGMAP* CMissingProbe::GetMessageMap() const { return &messageMap; }
''')
        self.assertFalse(any(r[3] == "data" for r in rows))
        self.assertTrue(any("did not bind both" in e for e in errors), errors)

    def test_preprocessed_out_annotation_is_not_silently_lost(self):
        rows, errors = self.extract('''
#if 0
DATA_MESSAGE_MAP(0x00200000, 0x00200008)
#endif
int g_unrelated;
''')
        self.assertEqual(rows, [])
        self.assertTrue(any("did not bind both" in e for e in errors), errors)

    def test_completeness_requires_both_data_addresses(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "src").mkdir()
            (root / "src/Probe.cpp").write_text(
                "DATA_MESSAGE_MAP(0x00200000, 0x00200008)\n")
            claim = SimpleNamespace(channel="src", kind="data", rva=0x200000)
            with mock.patch.object(source, "REPO", root), mock.patch(
                "gruntz.retail_labels.fragments.all_claims", return_value=[claim]
            ):
                errors = source.check_completeness()
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("0x200008", errors[0])

    def test_clang_mirror_changes_only_implicit_member_qualification(self):
        from gruntz.graph.compdb import adapt_mfc_message_macros
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            real, mirror = root / "sdk", root / "mirror"
            real.mkdir()
            mirror.mkdir()
            original = '#define ON_WM_PAINT() { WM_PAINT, 0, &OnPaint },\n'
            (real / "AFXMSG_.H").write_text(original)
            dest = mirror / "afxmsg_.h"
            dest.symlink_to(real / "AFXMSG_.H")
            adapt_mfc_message_macros(real, mirror)
            self.assertEqual((real / "AFXMSG_.H").read_text(), original)
            self.assertFalse(dest.is_symlink())
            self.assertEqual(dest.read_text(), original.replace("&OnPaint", "&MfcMessageMapClass::OnPaint"))
            stamp = dest.stat().st_mtime_ns
            adapt_mfc_message_macros(real, mirror)
            self.assertEqual(dest.stat().st_mtime_ns, stamp)


if __name__ == "__main__":
    unittest.main()
