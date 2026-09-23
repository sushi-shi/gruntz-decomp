"""Shared container ownership and real-VC5 public-header controls."""

import os
from pathlib import Path
import re
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
INCLUDE = ROOT / 'include'


class ContainerHeaderOwnershipTests(unittest.TestCase):
    def test_shared_headers_do_not_import_consumers(self):
        roots = [*sorted((INCLUDE / 'ZTools').glob('*.h')),
                 INCLUDE / 'Utils/FreeNodePool.h',
                 INCLUDE / 'Lith/TypedList.h',
                 INCLUDE / 'DinMgr2/InputDeviceGroup.h']
        self.assertTrue(list((INCLUDE / 'ZTools').glob('*.h')))
        for root in roots:
            with self.subTest(header=root.relative_to(INCLUDE)):
                pending, seen = [root], set()
                while pending:
                    header = pending.pop()
                    if header in seen:
                        continue
                    seen.add(header)
                    for name in re.findall(r'^\s*#include\s*[<"]([^>"]+)[>"]',
                                           header.read_text(), re.M):
                        self.assertFalse(name.startswith(('Gruntz/', 'Bute/', 'Wap32/')),
                                         f'{header.relative_to(INCLUDE)} imports {name}')
                        dependency = INCLUDE / name
                        if dependency.is_file():
                            pending.append(dependency)


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class Vc5ContainerHeaderTests(unittest.TestCase):
    def compile(self, source):
        from gruntz.tool import cl
        with tempfile.TemporaryDirectory(prefix='gruntz-container-header-') as directory:
            folder = Path(directory)
            path = folder / 'probe.cpp'
            path.write_text(source)
            return cl.compile(path, folder / 'probe.obj', ['/nologo', '/c', '/O2', '/MT', '/GX'])

    def test_array_header_exposes_the_complete_template(self):
        self.compile('''#include <ZTools/ZDArray.h>
typedef char base_size[sizeof(_zvec) == 28 ? 1 : -1];
typedef char dynamic_size[sizeof(_zdvec) == 36 ? 1 : -1];
typedef char array_size[sizeof(zDArray<int>) == 36 ? 1 : -1];
void use() { zDArray<int> values(0, 3); values[1] = 7; }
''')

    def test_symbol_table_header_has_no_bute_or_game_prerequisite(self):
        self.compile('''#include <ZTools/PTree.h>
typedef char table_size[sizeof(zSymTab<int>) == 44 ? 1 : -1];
int* use(int* value) {
    zSymTab<int> values(zPtrColl::NONE);
    values.add("value", value);
    return values.lookup("value");
}
''')

    def test_ztools_placement_new_coexists_with_standard_placement(self):
        for headers in (('#include <new>', '#include <ZTools/ZDArray.h>'),
                        ('#include <ZTools/ZDArray.h>', '#include <new>')):
            with self.subTest(headers=headers):
                self.compile('\n'.join(headers) + '''
struct Item { Item(); ~Item(); };
Item* custom(void* storage) { return new (storage, 0, 0) Item; }
Item* standard(void* storage) { return new (storage) Item; }
Item& index(zDArray<Item>& values, int i) { return values[i]; }
void lifetime() { zDArray<Item> values(0, 3); }
''')

    def test_bit_vector_header_is_self_contained(self):
        self.compile('''#include <ZTools/BitVec.h>
int use(zBitVec& bits) { return bits.GetBit(3); }
''')

    def test_pool_layout_and_payload_are_independent_of_coord(self):
        self.compile('''#include <Utils/FreeNodePool.h>
struct Payload { int x, y; };
typedef char pool_size[sizeof(FreeNodePool<Payload>) == 16 ? 1 : -1];
typedef char node_size[sizeof(FreeNodePool<Payload>::Node) == 12 ? 1 : -1];
typedef char payload_offset[offsetof(FreeNodePool<Payload>::Node, m_value) == 4 ? 1 : -1];
void use(FreeNodePool<Payload>& pool, Payload* value) { pool.Push(value); }
''')

    def test_input_array_layout(self):
        self.compile('''#include <DinMgr2/InputDeviceGroup.h>
#include <stddef.h>
typedef char array_size[sizeof(CInputDeviceGroup) == 136 ? 1 : -1];
typedef char slots_offset[offsetof(CInputDeviceGroup, m_items) == 8 ? 1 : -1];
int use(CInputDeviceGroup& group, CInputDevBase* value) { return group.Add(value); }
''')

    def test_action_pmf_cast_preserves_primary_base_method_identity(self):
        from gruntz.tool import cl
        from gruntz.delink.coffx import Obj
        from gruntz.walls.pairscan import functions, fn_relocs
        with tempfile.TemporaryDirectory(prefix='gruntz-action-pmf-') as directory:
            folder = Path(directory)
            source = folder / 'probe.cpp'
            source.write_text('''#include <Gruntz/Grunt.h>
typedef char base_width[sizeof(CActHandler) == 4 ? 1 : -1];
typedef char derived_width[sizeof(GruntActHandler) == 8 ? 1 : -1];
CActHandler constant() {
    return static_cast<CActHandler>(&CGrunt::FinishEntranceMove);
}
CActHandler parameter(GruntActHandler handler) {
    return static_cast<CActHandler>(handler);
}
''')
            output = folder / 'probe.obj'
            cl.compile(source, output, ['/nologo', '/c', '/O2', '/MT', '/GX'])
            obj = Obj(output)
            bodies = functions(obj)
            for caller, code, refs in (
                ('constant', bytes.fromhex('b8 00 00 00 00 c3'),
                 [(1, '?FinishEntranceMove@CGrunt@@QAEHXZ', 6, 0)]),
                ('parameter', bytes.fromhex('8b 44 24 04 c3'), []),
            ):
                with self.subTest(caller=caller):
                    name = next(n for n in bodies if n.startswith('?' + caller + '@@'))
                    section, start, end = bodies[name]
                    body = obj.section_payload(section)[start:end]
                    self.assertEqual(body[:len(code)], code)
                    self.assertTrue(all(b == 0x90 for b in body[len(code):]))
                    actual = [(offset-start, name, kind, addend)
                              for offset, name, kind, addend
                              in fn_relocs(obj, section, start, end)]
                    self.assertEqual(actual, refs)

    def test_action_pmf_cast_rejects_an_unrelated_owner(self):
        from gruntz.tool import ToolError
        with self.assertRaisesRegex(ToolError, 'cannot convert'):
            self.compile('''#include <Gruntz/Grunt.h>
CActHandler unrelated(i32 (CString::*handler)()) {
    return static_cast<CActHandler>(handler);
}
''')

    def test_input_array_rejects_a_different_element_type(self):
        from gruntz.tool import ToolError
        with self.assertRaisesRegex(ToolError, 'cannot convert'):
            self.compile('''#include <DinMgr2/InputDeviceGroup.h>
struct Unrelated {};
int use(CInputDeviceGroup& group, Unrelated* value) { return group.Add(value); }
''')

    def test_input_group_header_preserves_out_of_line_helpers(self):
        from gruntz.tool import cl
        from gruntz.delink.coffx import Obj
        from gruntz.walls.pairscan import functions, fn_relocs
        with tempfile.TemporaryDirectory(prefix='gruntz-input-group-') as directory:
            folder = Path(directory)
            source = folder / 'probe.cpp'
            source.write_text('''#include <DinMgr2/InputDeviceGroup.h>
int fill(CInputDeviceGroup& g, CInputDevBase** values, int n) {
    return g.FillFrom(values, n, 0);
}
void clear(CInputDeviceGroup& g) { g.Clear(); }
int add(CInputDeviceGroup& g, CInputDevBase* value) { return g.Add(value); }
''')
            output = folder / 'probe.obj'
            cl.compile(source, output, ['/nologo', '/c', '/O2', '/MT'])
            obj = Obj(output)
            bodies = functions(obj)
            for caller, callee in (('fill', 'FillFrom'), ('clear', 'Clear'),
                                   ('add', 'Add')):
                with self.subTest(helper=callee):
                    name = next(n for n in bodies if n.startswith('?' + caller + '@@'))
                    section, start, end = bodies[name]
                    refs = [r[1] for r in fn_relocs(obj, section, start, end)]
                    self.assertEqual(len(refs), 1, refs)
                    self.assertTrue(refs[0].startswith('?' + callee + '@CInputDeviceGroup@@'),
                                    refs)

    def test_list_keeps_its_erased_base_and_typed_access(self):
        self.compile('''#include <Lith/TypedList.h>
struct Item : CBaseListItem { int value; };
typedef char list_size[sizeof(CLTList<Item>) == 8 ? 1 : -1];
Item* use(CLTList<Item>& list, Item* item) {
    list.InsertFirst(item);
    return list.GetFirst();
}
''')

    def test_brickz_pools_have_distinct_storage_policies(self):
        self.compile('''#include <Gruntz/MapMgr.h>
#include <stddef.h>
typedef char node_extent[sizeof(CBrickzNodePool) == 12 ? 1 : -1];
typedef char cell_extent[sizeof(CBrickzCellNodePool) == 12 ? 1 : -1];
typedef char node_storage[offsetof(CBrickzNodePool, m_storage) == 4 ? 1 : -1];
typedef char node_head[offsetof(CBrickzNodePool, m_freeList) == 0 ? 1 : -1];
typedef char cell_storage[offsetof(CBrickzCellNodePool, m_storage) == 0 ? 1 : -1];
typedef char cell_head[offsetof(CBrickzCellNodePool, m_freeList) == 4 ? 1 : -1];
''')

    def test_sdk_typed_pointer_collections_change_vtable_identity(self):
        from gruntz.tool import cl
        from gruntz.delink.coffx import Obj
        from gruntz.walls.pairscan import functions, fn_relocs
        # Constructor calls alone cannot distinguish a typed SDK subclass:
        # the typed constructor first calls the same native base constructor.
        with tempfile.TemporaryDirectory(prefix='gruntz-mfc-identity-') as directory:
            folder = Path(directory)
            source = folder / 'probe.cpp'
            source.write_text('''#include <Mfc.h>
#include <afxtempl.h>
CPtrArray* NativePtrArray() { return new CPtrArray; }
CObArray* NativeObArray() { return new CObArray; }
CPtrList* NativePtrList() { return new CPtrList; }
CObList* NativeObList() { return new CObList; }
CTypedPtrArray<CPtrArray, int*>* TypedPtrArray() { return new CTypedPtrArray<CPtrArray, int*>; }
CTypedPtrArray<CObArray, CObject*>* TypedObArray() { return new CTypedPtrArray<CObArray, CObject*>; }
CTypedPtrList<CPtrList, int*>* TypedPtrList() { return new CTypedPtrList<CPtrList, int*>; }
CTypedPtrList<CObList, CObject*>* TypedObList() { return new CTypedPtrList<CObList, CObject*>; }
CMapStringToPtr* NativeStringMap() { return new CMapStringToPtr; }
CMapPtrToPtr* NativeIdMap() { return new CMapPtrToPtr; }
CTypedPtrMap<CMapStringToPtr, CString, void*>* TypedStringMap() { return new CTypedPtrMap<CMapStringToPtr, CString, void*>; }
CTypedPtrMap<CMapPtrToPtr, void*, void*>* TypedIdMap() { return new CTypedPtrMap<CMapPtrToPtr, void*, void*>; }
''')
            output = folder / 'probe.obj'
            cl.compile(source, output, ['/nologo', '/c', '/O2', '/MT', '/GX'])
            obj = Obj(output)
            bodies = functions(obj)
            for suffix in ('PtrArray', 'ObArray', 'PtrList', 'ObList', 'StringMap', 'IdMap'):
                with self.subTest(collection=suffix):
                    for kind in ('Native', 'Typed'):
                        name = next(n for n in bodies if n.startswith('?' + kind + suffix + '@@'))
                        section, start, end = bodies[name]
                        refs = [r[1] for r in fn_relocs(obj, section, start, end)]
                        derived = [r for r in refs if r.startswith('??_7?$CTypedPtr')]
                        self.assertEqual(bool(derived), kind == 'Typed', refs)

    def test_native_map_lookup_identities_reach_model(self):
        from gruntz.model import resolve
        from gruntz.sema.image import retail
        functions = {b.rva: b for b in resolve().functions}
        image = retail()
        # The instructions distinguish the stored key at +8 from value at +12.
        # Check the consumed Model, not just the label recognizer/input row.
        for address, name, offset in (
            (0x1b8438, '?Lookup@CMapStringToPtr@@QBEHPBDAAPAX@Z', 12),
            (0x1b845a, '?LookupKey@CMapStringToPtr@@QBEHPBDAAPBD@Z', 8),
        ):
            with self.subTest(address=hex(address)):
                self.assertEqual(functions[address].name, name)
                body = image.read(address, functions[address].size)
                self.assertEqual(body[0x16:0x19], bytes((0x8b, 0x40, offset)))
                self.assertNotEqual(body[0x16:0x19], bytes((0x8b, 0x40, 20-offset)))


if __name__ == '__main__':
    unittest.main()
