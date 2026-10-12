"""Validate saved T15 actors, not only freshly spawned Blueprint instances."""
import json
from pathlib import Path
import unreal

MAP_PATH = '/Game/MothEffect/Tests/L_T15Drops'
NAMES = ('Launcher', 'Bomb', 'Emitter')
KINDS = (unreal.DeviceKind.LAUNCHER, unreal.DeviceKind.BOMB, unreal.DeviceKind.EMITTER)


def validate_placed_enemies(world, repair=False):
    if world.get_path_name().split('.')[0] != MAP_PATH:
        raise RuntimeError('Refuse to validate/repair a different map')
    if repair:
        marker = Path(unreal.Paths.project_saved_dir()) / 'Automation/T15/CreatedPackages.json'
        if not marker.exists() or MAP_PATH not in json.loads(marker.read_text(encoding='utf-8')):
            raise RuntimeError('Refuse to repair a map not owned by the T15 generator')
    placed = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EnemyBase)
    if len(placed) != 3:
        raise RuntimeError(f'T15 range must contain three saved enemies, found {len(placed)}')
    records = []
    for name, kind in zip(NAMES, KINDS):
        cls = unreal.load_class(None, f'/Game/MothEffect/Enemies/BP_Enemy{name}.BP_Enemy{name}_C')
        drop = unreal.load_class(None, f'/Game/MothEffect/Devices/Drops/BP_Drop{name}.BP_Drop{name}_C')
        matches = [enemy for enemy in placed if enemy.get_class() == cls]
        if len(matches) != 1:
            raise RuntimeError('Missing or duplicate saved enemy: ' + name)
        enemy = matches[0]
        if repair and (enemy.get_editor_property('drop_device_class') != drop or enemy.get_drop_kind() != kind):
            # Drop configuration is EditDefaultsOnly. Replace only the stale owned
            # actor with a fresh instance of the saved class, preserving its placement.
            actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
            replacement = actors.spawn_actor_from_class(cls, enemy.get_actor_location(), enemy.get_actor_rotation())
            if not replacement or replacement.get_editor_property('drop_device_class') != drop or replacement.get_drop_kind() != kind:
                raise RuntimeError('Fresh saved enemy did not inherit fixed drop defaults: ' + name)
            replacement.set_actor_scale3d(enemy.get_actor_scale3d())
            replacement.set_actor_label(enemy.get_actor_label())
            replacement.set_editor_property('tags', enemy.get_editor_property('tags'))
            if not actors.destroy_actor(enemy):
                raise RuntimeError('Cannot remove stale generator-owned enemy: ' + name)
            enemy = replacement
        defaults = unreal.get_default_object(cls)
        for obj in (defaults, enemy):
            if obj.get_editor_property('drop_device_class') != drop or obj.get_drop_kind() != kind:
                raise RuntimeError('Saved drop configuration mismatch: ' + obj.get_path_name())
        records.append({'enemy': enemy.get_path_name(), 'kind': str(kind), 'drop': drop.get_path_name()})
    return records


if __name__ == '__main__':
    repair = '-MothRepairT15Drops' in unreal.SystemLibrary.get_command_line()
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    records = validate_placed_enemies(world, repair=repair)
    if repair:
        if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
            raise RuntimeError('Cannot save repaired T15 map')
        world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
        records = validate_placed_enemies(world)
    unreal.log('T15_PLACED_DROPS_VERIFIED ' + json.dumps({'repaired': repair, 'actors': records}))
