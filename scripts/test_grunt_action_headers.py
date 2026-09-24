"""Real-VC5 header and source-boundary controls; never launches the game."""

import os
from pathlib import Path
import tempfile
import unittest


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class GruntActionHeaderTests(unittest.TestCase):
    def test_complete_action_header_and_ordered_boundaries(self):
        from gruntz.delink.coffx import Obj
        from gruntz.tool import cl
        from gruntz.walls.pairscan import functions, fn_relocs

        with tempfile.TemporaryDirectory(prefix='gruntz-action-header-') as directory:
            root = Path(directory)
            source = root / 'probe.cpp'
            source.write_text('''#include <Gruntz/GruntActionInline.h>
class ActionProbe : public CGrunt {
public:
    bool ToyUse() { bool result; return GRUNT_IS_USING_TOY(result); }
    bool TubeMove() { bool result; return SETTLE_ACTIVE_TUBE_MOVE(result); }
    bool EntranceAppearance() { bool result; return APPLY_ACTIVE_ENTRANCE_PICKUP(result); }
    bool BombRun() { bool result; return TERMINATE_ACTIVE_BOMB_RUN(result); }
};
bool use(ActionProbe* grunt, int defer) {
    grunt->RestorePreviousAppearance();
    grunt->RestoreToolAfterToyUse(defer);
    grunt->ApplyEntrancePickup();
    return grunt->ToyUse() || grunt->TubeMove() || grunt->EntranceAppearance()
        || grunt->SettleActiveKnockback() || grunt->BombRun();
}
''')
            obj_path = root / 'probe.obj'
            cl.compile(source, obj_path, ['/nologo', '/c', '/O2', '/Ob0', '/MT', '/GX'])
            obj = Obj(obj_path)
            bodies = functions(obj)

            def calls(name):
                section, start, end = bodies[name]
                return [target for _, target, kind, _ in fn_relocs(obj, section, start, end)
                        if kind == 0x14]

            settle = calls('?SettleTubeMove@CGrunt@@QAEXXZ')
            required = ['?IsDropReady@CGrunt@@QAEHH@Z',
                        '?SnapToLastTile@CGrunt@@QAEXH@Z',
                        '?SetupTubeAnim@CGrunt@@QAEHH@Z']
            # Check the actual emitted signature as well as source ordering.
            self.assertEqual([name for name in settle if name in required], required)
            self.assertEqual(calls('?TubeMove@ActionProbe@@QAE_NXZ'), [
                '?IsAnimationAct@CUserLogic@@QBE_NPBD@Z',
                '?SettleTubeMove@CGrunt@@QAEXXZ'])
            self.assertEqual(calls('?EntranceAppearance@ActionProbe@@QAE_NXZ'), [
                '?IsAnimationAct@CUserLogic@@QBE_NPBD@Z',
                '?RestorePreviousAppearance@CGrunt@@QAEXXZ',
                '?ApplyEntrancePickup@CGrunt@@QAEXXZ'])
            self.assertEqual(calls('?SettleActiveKnockback@CGrunt@@QAE_NXZ'), [
                '?IsAnimationAct@CUserLogic@@QBE_NPBD@Z',
                '?SettleKnockback@CGrunt@@QAEXXZ'])
            self.assertEqual(calls('?SettleKnockback@CGrunt@@QAEXXZ'), [
                '?SnapToLastTile@CGrunt@@QAEXH@Z',
                '?WireTileSwitchLogic@CTriggerMgr@@QAEHPAVCGrunt@@HH@Z'])
            bomb = calls('?BombRun@ActionProbe@@QAE_NXZ')
            self.assertIn('?GetAnimationActName@CUserLogic@@QBEABVCString@@XZ', bomb)
            self.assertEqual(bomb[-1],
                '?StartUnitDeath@CTriggerMgr@@QAEHHHW4GruntDeathType@@H@Z')
            pickup = calls('?ApplyEntrancePickup@CGrunt@@QAEXXZ')
            self.assertEqual(pickup, [
                '?LoadGruntTypeTable@CGrunt@@QAEHW4PickupType@@HHH@Z',
                '?LoadVehicleGruntSprites@CGrunt@@QAEHW4PickupType@@@Z',
                '?LoadGruntTypeTable@CGrunt@@QAEHW4PickupType@@HHH@Z'])
            self.assertEqual(calls('?ToyUse@ActionProbe@@QAE_NXZ'), [
                '?IsAnimationAct@CUserLogic@@QBE_NPBD@Z'] * 3)
            appearance = calls('?RestorePreviousAppearance@CGrunt@@QAEXXZ')
            self.assertIn('?EntranceCell@CGrunt@@QAEPAUCGruntCellRec@@XZ', appearance)
            self.assertIn('?SetImageSetByName@CWapX@@QAEXPBD@Z', appearance)
            restore = calls('?RestoreToolAfterToyUse@CGrunt@@QAEXH@Z')
            self.assertIn('?LoadGruntTypeTable@CGrunt@@QAEHW4PickupType@@HHH@Z', restore)
            self.assertIn('?StopVehicleLoopSound@CGrunt@@QAEXXZ', restore)


if __name__ == '__main__':
    unittest.main()
