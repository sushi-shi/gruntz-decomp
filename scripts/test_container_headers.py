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
                 INCLUDE / 'Utils/FixedPtrArray.h',
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
typedef char array_size[sizeof(CInputDeviceGroup) == 136 ? 1 : -1];
typedef char slots_offset[offsetof(CInputDeviceGroup, m_items) == 8 ? 1 : -1];
int use(CInputDeviceGroup& group, CInputDevBase* value) { return group.Add(value); }
''')

    def test_input_array_rejects_a_different_element_type(self):
        from gruntz.tool import ToolError
        with self.assertRaisesRegex(ToolError, 'cannot convert'):
            self.compile('''#include <DinMgr2/InputDeviceGroup.h>
struct Unrelated {};
int use(CInputDeviceGroup& group, Unrelated* value) { return group.Add(value); }
''')

    def test_list_keeps_its_erased_base_and_typed_access(self):
        self.compile('''#include <Lith/TypedList.h>
struct Item : CBaseListItem { int value; };
typedef char list_size[sizeof(CLTList<Item>) == 8 ? 1 : -1];
Item* use(CLTList<Item>& list, Item* item) {
    list.InsertFirst(item);
    return list.GetFirst();
}
''')


if __name__ == '__main__':
    unittest.main()
