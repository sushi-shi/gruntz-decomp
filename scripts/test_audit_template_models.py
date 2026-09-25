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
    def test_mfc_member_inventory_keeps_arrays_and_does_not_certify_templates(self):
        rows = audit.mfc_pointer_members({
            'owner': dict(name='Owner', owned=True, file='Owner.h', fields=[
                dict(name='items', type='CPtrList[8]', offset=352, size=224),
                dict(name='values', type='CObArray', offset=2144, size=20),
                dict(name='borrowed', type='CPtrList *', offset=2304, size=4)]),
            'sdk': dict(name='SDK', owned=False, fields=[
                dict(name='list', type='CPtrList', offset=0, size=28)])})
        self.assertEqual([(r['member'], r['base'], r['count'], r['offset_bits']) for r in rows],
                         [('items', 'CPtrList', 8, 352), ('values', 'CObArray', 1, 2144)])
        self.assertTrue(all(r['status'] == 'requires-constructor-and-use-family-audit' for r in rows))

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

    def test_parser_change_invalidates_the_saved_census(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'scripts').mkdir()
            parser = root / 'scripts/audit-template-models.py'
            parser.write_text('parser version one\n')
            with patch.object(audit, 'ROOT', root):
                (root / 'functions.json').write_text('{"fn": {"source_hash": "abc"}}')
                import json
                (root / 'snapshot.json').write_text(json.dumps(audit.snapshot()))
                parser.write_text('parser version two\n')
                with self.assertRaisesRegex(SystemExit, 'changed since census'):
                    audit.write_queue(root, root / 'reviews.tsv')

    def test_implicit_member_remains_reviewable_without_a_coff_emission(self):
        record = dict(owned=True, name='Widget', methods=[], source_hash='owner',
            bases=[], fields=[], file=str(audit.ROOT / 'include/Widget.h'), line=1)
        rows, issues = self.queue(records={'widget': record})
        implicit = [r for r in rows if r['category'] == 'implicit-special-member']
        self.assertFalse(issues)
        self.assertEqual({r['key'] for r in implicit},
            {'implicit:widget:constructor', 'implicit:widget:destructor'})
        self.assertEqual({r['status'] for r in implicit}, {'implicit-declaration'})
        ctor = next(r for r in implicit if r['key'].endswith(':constructor'))
        review = dict(fingerprint=ctor['fingerprint'], disposition='recovered-implicit',
            evidence='Actual caller omission control preserves the complete body.')
        rows, issues = self.queue(records={'widget': record},
            reviews={ctor['key']: review})
        self.assertFalse(issues)
        self.assertEqual(next(r for r in rows if r['key'] == ctor['key'])['status'],
            'recovered-implicit')

    def test_any_authored_constructor_suppresses_implicit_default_candidate(self):
        record = dict(owned=True, name='Widget', methods=['Widget', '~Widget'],
            source_hash='owner', bases=[], fields=[],
            file=str(audit.ROOT / 'include/Widget.h'), line=1)
        rows, _ = self.queue(records={'widget': record})
        self.assertFalse(any(r['category'] == 'implicit-special-member' for r in rows))


@unittest.skipUnless(importlib.util.find_spec('clang'), 'requires the pinned Nix clang environment')
class AstFingerprintTests(unittest.TestCase):
    def test_anonymous_union_is_not_an_independent_implicit_owner(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'src').mkdir()
            source = root / 'src/probe.cpp'
            source.write_text('struct Owner { union { int value; float alternate; }; '
                'struct { int x; }; }; union Named { int value; };\n')
            try:
                with patch.object(audit, 'ROOT', root):
                    audit.owned.cache_clear()
                    records, templates, _, errors, functions, uses = audit.harvest(
                        (str(source), ['/TP']))
                    self.assertFalse(errors)
                    self.assertEqual(sum(r['anonymous'] for r in records.values()), 2)
                    rows, issues = audit.review_queue(records, templates, functions,
                        [], {}, {}, uses, {})
                    self.assertFalse(issues)
                    implicit = [r for r in rows if r['category'] == 'implicit-special-member']
                    self.assertEqual({r['name'] for r in implicit}, {
                        'Owner::constructor', 'Owner::destructor',
                        'Named::constructor', 'Named::destructor'})
            finally:
                audit.owned.cache_clear()

    def test_union_conversion_body_has_a_complete_owner_fingerprint(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'src').mkdir()
            source = root / 'src/probe.cpp'
            try:
                with patch.object(audit, 'ROOT', root):
                    audit.owned.cache_clear()
                    fingerprints = []
                    for member in ('int value;', 'int value; long other;'):
                        source.write_text('union Choice { ' + member +
                            ' operator int() const { return value; } };\n')
                        records, templates, _, errors, functions, uses = audit.harvest(
                            (str(source), ['/TP']))
                        self.assertFalse(errors)
                        self.assertTrue(any(r['kind'] == 'UNION_DECL' for r in records.values()))
                        for row in functions.values():
                            row['bindings'] = []
                        rows, issues = audit.review_queue(records, templates, functions,
                            [], {}, {}, uses, {})
                        self.assertFalse(issues)
                        method = next(r for r in rows if r['category'] == 'source-definition')
                        self.assertIn('operator int()', method['name'])
                        fingerprints.append(method['fingerprint'])
                    self.assertNotEqual(*fingerprints)
            finally:
                audit.owned.cache_clear()

    def test_unused_template_bodies_are_in_the_complete_queue(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'src').mkdir()
            source = root / 'src/probe.cpp'
            source.write_text('template<class T> struct Holder { T* first; '
                'T* Used() { return first; } T* Unused() { return first; } }; '
                'template<class T> T NeverCalled(T a) { return a; } '
                'Holder<int> h; int* caller() { return h.Used(); }\n')
            try:
                with patch.object(audit, 'ROOT', root):
                    audit.owned.cache_clear()
                    records, templates, _, errors, functions, uses = audit.harvest(
                        (str(source), ['/TP', '-fdelayed-template-parsing']))
                    self.assertFalse(errors)
                    for row in functions.values():
                        row['bindings'] = []  # The isolated fixture has no retail claims.
                    rows, issues = audit.review_queue(records, templates, functions,
                        [], {}, {}, uses, {})
                    self.assertFalse(issues)
                    names = {r['name'] for r in rows if r['category'] == 'source-definition'}
                    self.assertTrue(any(n.endswith('::Unused()') for n in names), names)
                    self.assertTrue(any(n.endswith('::NeverCalled(T)') for n in names), names)
            finally:
                audit.owned.cache_clear()

    def test_actual_declarations_control_implicit_member_rows(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'src').mkdir()
            source = root / 'src/probe.cpp'
            try:
                with patch.object(audit, 'ROOT', root):
                    audit.owned.cache_clear()
                    for declarations, expected in [('', {'constructor', 'destructor'}),
                            ('Widget(int);', {'destructor'}),
                            ('template<class T> Widget(T);', {'destructor'}),
                            ('Widget(int); ~Widget();', set())]:
                        source.write_text('struct Widget { ' + declarations + ' int value; };\n')
                        records, templates, _, errors, functions, uses = audit.harvest(
                            (str(source), ['/TP']))
                        self.assertFalse(errors)
                        rows, issues = audit.review_queue(records, templates, functions,
                            [], {}, {}, uses, {})
                        self.assertFalse(issues)
                        self.assertEqual({r['key'].rsplit(':', 1)[1] for r in rows
                            if r['category'] == 'implicit-special-member'}, expected)
            finally:
                audit.owned.cache_clear()

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
