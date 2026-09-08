"""Controls for exhaustive selection and source-scoped review invalidation."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('audit', Path(__file__).with_name('audit-template-models.py'))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class ReviewQueueTests(unittest.TestCase):
    def function(self, **changes):
        row = dict(kind='CXX_METHOD', owner='Widget', owner_usr='widget', name='Tick()',
            empty_body=False, definition_tokens=['void', 'Tick', '(', ')', '{', '}'],
            tokens=['{', '}'], source_hash='body', template_context=False,
            file=str(audit.ROOT / 'src/Widget.cpp'), line=1, bindings=[], symbol='?Tick', variants=[])
        row.update(changes)
        return row

    def queue(self, function=None, records=None, reviews=None):
        return audit.review_queue(records or {}, {}, {'fn': function or self.function()},
            [], {}, {}, [], reviews or {})

    def test_unremarkable_method_stays_in_exhaustive_queue(self):
        rows, issues = self.queue()
        self.assertEqual(rows[0]['status'], 'pending')
        self.assertEqual(rows[0]['signals'], 'exhaustive-fallback')
        self.assertFalse(issues)

    def test_empty_ctor_with_initializer_is_not_certified_generated(self):
        row = self.function(kind='CONSTRUCTOR', empty_body=True,
            definition_tokens=[], initializers=[dict(tokens=['Base', '(', 'INLINE_BASE', ')'])])
        rows, _ = self.queue(row)
        self.assertEqual(rows[0]['status'], 'pending')
        self.assertIn('emission-selector', rows[0]['signals'])
        self.assertEqual(rows[0]['priority'], 0)

    def test_lifetime_signal_does_not_confuse_bitwise_not_or_qualified_call(self):
        for tokens in (['{', 'x', '=', '~', 'mask', ';', '}'],
                       ['{', 'p', '->', 'Base', '::', 'Update', '(', ')', ';', '}']):
            rows, _ = self.queue(self.function(tokens=tokens))
            self.assertNotIn('explicit-lifetime', rows[0]['signals'])
        for tokens in (['{', 'p', '->', 'T', '::', '~', 'T', '(', ')', ';', '}'],
                       ['{', 'p', '->', 'CString', '::', 'CString', '(', ')', ';', '}']):
            rows, _ = self.queue(self.function(tokens=tokens))
            self.assertIn('explicit-lifetime', rows[0]['signals'])

    def test_owner_change_reopens_unchanged_method(self):
        records = {'widget': dict(source_hash='owner-before', bases=[], fields=[])}
        rows, _ = self.queue(records=records)
        review = dict(fingerprint=rows[0]['fingerprint'], disposition='retained-authored', evidence='Retail call and stores.')
        rows, _ = self.queue(records=records, reviews={'method:fn': review})
        self.assertEqual(rows[0]['status'], 'retained-authored')
        records['widget']['source_hash'] = 'owner-after'
        rows, _ = self.queue(records=records, reviews={'method:fn': review})
        self.assertEqual(rows[0]['status'], 'stale-review')

    def test_removed_method_review_is_not_silent_completion(self):
        _, issues = self.queue(reviews={'method:removed': {}})
        self.assertIn('Orphan review', issues[0])

    def test_same_usr_with_different_tu_definition_is_not_overwritten(self):
        functions = {}
        audit.merge_functions(functions, {'fn': self.function()})
        audit.merge_functions(functions, {'fn': self.function(source_hash='different', line=20)})
        rows, _ = self.queue(functions['fn'])
        self.assertIn('multiple-definition-forms', rows[0]['signals'])
        self.assertEqual(rows[0]['priority'], 0)
        self.assertEqual(len(functions['fn']['definition_sites']), 2)

    def test_macro_environment_change_reopens_without_claiming_odr_conflict(self):
        functions = {}
        audit.merge_functions(functions, {'fn': self.function(context_hash='macro-before')})
        rows, _ = self.queue(functions['fn'])
        review = dict(fingerprint=rows[0]['fingerprint'], disposition='retained-authored', evidence='Compiled control.')
        audit.merge_functions(functions, {'fn': self.function(context_hash='macro-after')})
        rows, _ = self.queue(functions['fn'], reviews={'method:fn': review})
        self.assertEqual(rows[0]['status'], 'stale-review')
        self.assertNotIn('multiple-definition-forms', rows[0]['signals'])

    def test_template_and_compiler_emission_are_independent_surfaces(self):
        rows, _ = audit.review_queue({}, {}, {'fn': self.function(template_context=True)},
            [], {'??_GWidget': [dict(unit='widget', offset=0, object_hash='abc')]}, {}, [], {})
        self.assertEqual({r['status'] for r in rows}, {'template-source', 'emitted-without-owned-body'})

    def test_queue_only_rejects_stale_census_before_applying_reviews(self):
        with tempfile.TemporaryDirectory() as d:
            output = Path(d)
            (output / 'functions.json').write_text('{"fn": {"source_hash": "abc"}}')
            (output / 'snapshot.json').write_text('{"src/Widget.cpp": "old"}')
            with patch.object(audit, 'snapshot', return_value={'src/Widget.cpp': 'new'}):
                with self.assertRaisesRegex(SystemExit, 'changed since census'):
                    audit.write_queue(output, output / 'reviews.tsv')


@unittest.skipUnless(importlib.util.find_spec('clang'), 'requires the pinned Nix clang environment')
class AstFingerprintTests(unittest.TestCase):
    def test_macro_extent_fallback_tracks_body_and_initializer(self):
        original_root = audit.ROOT
        original_tokens = audit.source_tokens

        def without_definition_extent(cursor):
            # Real RVA attributes have produced this empty stream. Exercise
            # its consumer through an actual parsed ctor, not a mocked row.
            return [] if cursor.kind.name == 'CONSTRUCTOR' else original_tokens(cursor)

        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'src').mkdir()
            source = root / 'src/probe.cpp'
            hashes = []
            try:
                with patch.object(audit, 'ROOT', root), patch.object(
                        audit, 'source_tokens', side_effect=without_definition_extent):
                    audit.owned.cache_clear()
                    for initializer, addend in ((1, 2), (1, 3), (2, 3)):
                        source.write_text('#include <rva.h>\nstruct Sample { Sample(); int value; };\n'
                            f'RVA(0x1234, 0x10)\nSample::Sample() : value({initializer}) '
                            f'{{ value += {addend}; }}\n')
                        _, _, _, errors, functions, _ = audit.harvest((str(source), [
                            '/TP', '/I' + str(original_root / 'include')]))
                        self.assertFalse(errors)
                        constructor = next(r for r in functions.values() if r['kind'] == 'CONSTRUCTOR')
                        self.assertEqual(constructor['definition_tokens'], [])
                        hashes.append(constructor['source_hash'])
            finally:
                audit.owned.cache_clear()
            self.assertEqual(len(set(hashes)), 3)


if __name__ == '__main__':
    unittest.main()
