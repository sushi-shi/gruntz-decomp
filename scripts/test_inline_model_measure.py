"""Real-VC5 control: a saturated harness does not prove a free inline."""

from contextlib import redirect_stdout
import io
import os
from pathlib import Path
import re
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class InlineMeasurementTests(unittest.TestCase):
    def test_saturated_measurement_does_not_claim_budget_exemption(self):
        from gruntz.walls import inline_model

        with tempfile.TemporaryDirectory(prefix='gruntz-span-measure-') as directory:
            folder = Path(directory)
            results = {}
            with patch.object(inline_model, '_MODEL_SCRATCH', folder):
                for sites in (12, 25):
                    source = folder / ('span_%d.cpp' % sites)
                    source.write_text('''#include <Gruntz/Minimap.h>
inline void CMinimap::FillSpan(u32 first, u32 last, u16 color) {
    if (first > last) return;
    for (u32 tile = first; tile <= last; tile++) {
        m_tileColors[tile] = color;
    }
}
void SpanCostProbe(CMinimap* map, u32 first, u32 last, u16 color) {
''' + '    map->FillSpan(first, last, color);\n' * sites + '}\n')
                    output = io.StringIO()
                    with redirect_stdout(output):
                        result = inline_model.main([
                            '--measure-cb', str(source),
                            '--fn', '?FillSpan@CMinimap@@QAEXIIG@Z',
                            '--caller', 'SpanCostProbe', '--sites', str(sites),
                        ])
                    self.assertEqual(result, 0)
                    results[sites] = output.getvalue()

            self.assertIn('12 expanded, 0 rejected', results[12])
            self.assertIn('SATURATED: no cb bound measured', results[12])
            self.assertNotIn('budget-exempt', results[12])
            self.assertNotIn('cb <=', results[12])
            measured = re.search(r'cb in \[(\d+),(\d+)\]', results[25])
            self.assertIsNotNone(measured, results[25])
            self.assertGreater(int(measured.group(1)), inline_model.SMALL_FREE)
            self.assertGreaterEqual(int(measured.group(2)), int(measured.group(1)))


if __name__ == '__main__':
    unittest.main()
