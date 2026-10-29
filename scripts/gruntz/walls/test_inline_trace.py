"""The c2 inline-trace parser and the traced back end's identity."""

import os
from pathlib import Path
import tempfile
import unittest

from gruntz.walls import inline_trace

# The trace of a three-function control (root -> mid -> 2x leaf, root -> leaf)
# as the shim writes it.
NESTED = """begin
sym 00ebb580 ?rootCharlie@@YAHH@Z
main 00ebb580 cb=27 handle=af flags=2a
sym 00ebb498 ?midBravo@@YAHH@Z
sym 00ebb580 ?rootCharlie@@YAHH@Z
site root=00ebb580 owner=00ebb580 callee=00ebb498 tuple=012051fc depth=1 allow=8 budget=1000 remain=2 cb=29 running=27 handle=ae flags=c8
args actual=1 formal=1
admit running=27
charge running=27
body callee=00ebb498 share=500 depth=2
sym 00ebb3b0 ?leafAlpha@@YAHH@Z
sym 00ebb498 ?midBravo@@YAHH@Z
site root=00ebb580 owner=00ebb498 callee=00ebb3b0 tuple=0120b1d8 depth=2 allow=8 budget=500 remain=2 cb=20 running=56 handle=ad flags=c8
args actual=1 formal=1
admit running=56
charge running=56
body callee=00ebb3b0 share=250 depth=3
site root=00ebb580 owner=00ebb498 callee=00ebb3b0 tuple=0120b2dc depth=2 allow=8 budget=500 remain=1 cb=20 running=76 handle=ad flags=c8
args actual=1 formal=1
admit running=76
charge running=76
body callee=00ebb3b0 share=500 depth=3
site root=00ebb580 owner=00ebb580 callee=00ebb3b0 tuple=01205300 depth=1 allow=8 budget=1000 remain=1 cb=20 running=96 handle=ad flags=c8
args actual=2 formal=1
end
"""

CONTROL = """inline int leafAlpha(int x) { return x * 3 + 1; }
inline int midBravo(int x) { return leafAlpha(x) + leafAlpha(x + 1); }
int rootCharlie(int x) { return midBravo(x) + leafAlpha(x); }
namespace { int hidden; }
static int zeroed[4];
int readHidden() { return hidden + zeroed[readHidden() & 3]; }
"""


class Parse(unittest.TestCase):
    def test_nested_sites_keep_order_owner_and_gates(self):
        roots = inline_trace.parse(NESTED)["roots"]
        self.assertEqual(len(roots), 1)
        root = roots[0]
        self.assertEqual((root["symbol"], root["cb"], root["initial_budget"]),
                         ("?rootCharlie@@YAHH@Z", 27, 1000))
        sites = root["sites"]
        self.assertEqual([s["symbol"] for s in sites],
                         ["?midBravo@@YAHH@Z", "?leafAlpha@@YAHH@Z",
                          "?leafAlpha@@YAHH@Z", "?leafAlpha@@YAHH@Z"])
        self.assertEqual([s["owner"] for s in sites],
                         ["?rootCharlie@@YAHH@Z", "?midBravo@@YAHH@Z",
                          "?midBravo@@YAHH@Z", "?rootCharlie@@YAHH@Z"])
        self.assertEqual([s["expanded"] for s in sites], [True, True, True, False])
        self.assertFalse(sites[3]["args_match"])
        self.assertEqual(sites[0]["share"], 500)

    def test_truncated_or_refused_trace_is_an_error(self):
        with self.assertRaises(ValueError):
            inline_trace.parse(NESTED.replace("end\n", ""))
        with self.assertRaises(ValueError):
            inline_trace.parse("refused c2.exe differs at a hook site\n")

    def test_body_without_admit_is_an_error(self):
        broken = NESTED.replace("admit running=27\n", "", 1)
        with self.assertRaises(ValueError):
            inline_trace.parse(broken)


@unittest.skipUnless(os.environ.get("MSVC_DIR"), "requires the pinned VC5 environment")
class TracedBackEnd(unittest.TestCase):
    def test_traced_compile_emits_the_plain_object(self):
        from gruntz.tool import cl
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            source = directory / "control.cpp"
            source.write_text(CONTROL)
            flags = ["/nologo", "/c", "/O2", "/MT"]
            plain = directory / "plain.obj"
            cl.compile(source, plain, flags)
            traced = directory / "traced.obj"
            text = inline_trace.compile_traced(source, flags, traced)
            # The anonymous namespace's per-compile number differs; nothing else may.
            self.assertEqual(inline_trace._comparable(plain.read_bytes()),
                             inline_trace._comparable(traced.read_bytes()))
            sites = inline_trace.parse(text)["roots"][0]["sites"]
            self.assertEqual([(s["depth"], s["budget"], s["expanded"]) for s in sites],
                             [(1, 1000, True), (2, 500, True), (2, 500, True), (1, 1000, True)])


if __name__ == "__main__":
    unittest.main()
