"""Generated Ninja dependencies must invalidate address comparison evidence."""

from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from gruntz import graph
from gruntz.graph import emit


class NormalizeDependencies(unittest.TestCase):
    def test_emitted_graph_tracks_manifest_producer_and_normalization_modules(self):
        manifest = {"flags": {"release": ["/O2"]}}
        unit = {"unit": "example", "source": "example.cpp", "cflags": ["/O2"]}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "build.ninja"
            with mock.patch.object(emit, "load_units", return_value=(manifest, [unit])), \
                    mock.patch.object(emit, "prune_orphan_artifacts", return_value=0), \
                    mock.patch.object(emit, "write_toolchain_id"), \
                    mock.patch.object(emit, "emit_link_phase"), \
                    mock.patch.object(emit, "Scanner") as scanner:
                scanner.return_value.headers.return_value = []
                scanner.return_value.scanned.return_value = set()
                emit.emit(path)
            query = subprocess.run(["ninja", "-f", str(path), "-t", "query",
                                    graph.NORMALIZE_STAMP, emit.DATA_MANIFEST],
                                   check=True, capture_output=True, text=True).stdout
        self.assertIn(emit.DATA_MANIFEST, query)
        for module in ("data_boundaries.py", "function_sizes.py", "normalize.py"):
            self.assertIn("scripts/gruntz/compare/" + module, query)
        manifest_query = query.split(emit.DATA_MANIFEST + ":", 1)[1]
        self.assertIn("input: delink", manifest_query)
        self.assertIn(graph.NORMALIZE_STAMP, manifest_query)


if __name__ == "__main__":
    unittest.main()
