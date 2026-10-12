"""Run with -ExecutePythonScript in a ticking editor: saved-map PIE death/drop regression."""
import json
from pathlib import Path
import sys
import time
import unreal

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / 'scripts'))
from t15_range_validation import MAP_PATH, NAMES, KINDS, validate_placed_enemies

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
validate_placed_enemies(world)
started = time.monotonic()
stage = 0
drops = []
records = []
death_time = 0
handle = None
cache = Path(unreal.Paths.project_saved_dir()) / 'Automation/T15'
cache.mkdir(parents=True, exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)


def finish(success, error=None):
    result = {'success': success, 'map': MAP_PATH, 'saved_enemies': records, 'error': error,
              'scope': 'NullRHI saved-map PIE with three native 20HP hits per enemy; graphical/manual acceptance pending'}
    (cache / 'PlacedDropRegression.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    if success:
        unreal.log('T15_PLACED_DROP_PIE_VERIFIED ' + json.dumps(result))
    else:
        unreal.log_error('T15_PLACED_DROP_PIE_FAILED ' + str(error))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.execute_console_command(world, 'QUIT_EDITOR')


def tick(delta):
    global stage, death_time
    try:
        if time.monotonic() - started > 45:
            raise RuntimeError('Timed out waiting for saved-map PIE/death/drop')
        if stage == 0:
            if time.monotonic() - started < 2:
                return
            levels.editor_play_simulate()
            stage = 1
            return
        pie = editor.get_game_world()
        if not pie:
            return
        if stage == 1:
            if unreal.GameplayStatics.get_time_seconds(pie) < .2:
                return
            enemies = unreal.GameplayStatics.get_all_actors_of_class(pie, unreal.EnemyBase)
            if len(enemies) != 3:
                raise RuntimeError('Expected exactly three original saved PIE enemies')
            for name, kind in zip(NAMES, KINDS):
                enemy = next(e for e in enemies if e.get_class().get_name() == 'BP_Enemy' + name + '_C')
                health = enemy.get_health_component()
                if health.get_current_health() != 60:
                    raise RuntimeError('Saved enemy must begin with 60HP')
                if not enemy.get_editor_property('backpack_mesh').is_visible():
                    raise RuntimeError('Saved enemy backpack must be visible: ' + name)
                for expected in (40, 20, 0):
                    context = unreal.HitContext(hit_id=unreal.GuidLibrary.new_guid(), damage=20)
                    if not health.apply_hit(context) or health.get_current_health() != expected:
                        raise RuntimeError('Three actual health hits must kill saved enemy: ' + name)
                drop = enemy.get_dropped_device()
                if not drop or drop.get_device_kind() != kind:
                    raise RuntimeError('Saved enemy did not spawn its matching drop: ' + name + ' result=' + str(enemy.get_drop_result()))
                if health.apply_hit(unreal.HitContext(hit_id=unreal.GuidLibrary.new_guid(), damage=20)):
                    raise RuntimeError('Dead enemy must reject further damage')
                drops.append(drop)
                records.append({'enemy': name, 'result': str(enemy.get_drop_result()),
                                'drop': drop.get_class().get_name(), 'kind': str(kind),
                                'birth_position': str(drop.get_actor_location())})
            death_time = unreal.GameplayStatics.get_time_seconds(pie)
            stage = 2
            return
        if unreal.GameplayStatics.get_time_seconds(pie) - death_time < .8:
            return
        devices = unreal.GameplayStatics.get_all_actors_of_class(pie, unreal.DeviceBase)
        if len(devices) != 3 or unreal.GameplayStatics.get_all_actors_of_class(pie, unreal.EnemyBase):
            raise RuntimeError('Exactly three drops must survive the three corpse cleanups')
        for drop in drops:
            if drop.get_device_state() != unreal.DeviceState.DORMANT or not drop.get_physics_body().is_simulating_physics():
                raise RuntimeError('Saved-map drops must remain normal physics-enabled Dormant devices')
        finish(True)
    except Exception as error:
        finish(False, str(error))


handle = unreal.register_slate_post_tick_callback(tick)
